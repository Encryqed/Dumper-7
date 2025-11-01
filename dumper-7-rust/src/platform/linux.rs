//! Linux platform implementation
//!
//! This module provides Linux-specific implementations using ptrace and /proc/maps

use super::{MemoryReader, MemoryRegion, ModuleInfo, Platform, PlatformError, PatternMatch, pattern_matches};
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::sync::Arc;
use parking_lot::RwLock;

/// Linux platform implementation
pub struct LinuxPlatform {
    /// Process ID we're reading from
    pid: i32,
    /// Cached memory regions
    regions_cache: Arc<RwLock<Option<Vec<MemoryRegion>>>>,
}

impl LinuxPlatform {
    /// Create a new Linux platform instance for the current process
    pub fn new() -> Result<Self, PlatformError> {
        Ok(Self {
            pid: unsafe { libc::getpid() },
            regions_cache: Arc::new(RwLock::new(None)),
        })
    }

    /// Attach to a specific process
    pub fn attach(pid: i32) -> Result<Self, PlatformError> {
        // Try to attach using ptrace
        let result = unsafe { libc::ptrace(libc::PTRACE_ATTACH, pid, 0, 0) };

        if result == -1 {
            return Err(PlatformError::AccessDenied(format!(
                "Failed to attach to process {}: {}",
                pid,
                std::io::Error::last_os_error()
            )));
        }

        Ok(Self {
            pid,
            regions_cache: Arc::new(RwLock::new(None)),
        })
    }

    /// Parse /proc/[pid]/maps to get memory regions
    fn parse_maps(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        let maps_path = format!("/proc/{}/maps", self.pid);
        let file = File::open(&maps_path)
            .map_err(|e| PlatformError::Other(format!("Failed to open {}: {}", maps_path, e)))?;

        let reader = BufReader::new(file);
        let mut regions = Vec::new();

        for line in reader.lines() {
            let line = line.map_err(|e| PlatformError::Other(format!("Failed to read maps: {}", e)))?;

            // Parse line format: "address permissions offset dev inode pathname"
            // Example: "7f1234567000-7f123456a000 r-xp 00000000 08:01 123456 /lib/x86_64-linux-gnu/libc.so.6"
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() < 2 {
                continue;
            }

            let address_range = parts[0];
            let perms = parts[1];

            // Parse address range
            let addresses: Vec<&str> = address_range.split('-').collect();
            if addresses.len() != 2 {
                continue;
            }

            let start = usize::from_str_radix(addresses[0], 16)
                .map_err(|e| PlatformError::Other(format!("Invalid address: {}", e)))?;

            let end = usize::from_str_radix(addresses[1], 16)
                .map_err(|e| PlatformError::Other(format!("Invalid address: {}", e)))?;

            // Parse permissions
            let readable = perms.chars().nth(0) == Some('r');
            let writable = perms.chars().nth(1) == Some('w');
            let executable = perms.chars().nth(2) == Some('x');

            regions.push(MemoryRegion {
                start,
                end,
                readable,
                writable,
                executable,
            });
        }

        Ok(regions)
    }

    /// Get memory regions, using cache if available
    fn get_regions_cached(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        let cache = self.regions_cache.read();
        if let Some(ref regions) = *cache {
            return Ok(regions.clone());
        }
        drop(cache);

        // Parse maps and cache
        let regions = self.parse_maps()?;
        *self.regions_cache.write() = Some(regions.clone());

        Ok(regions)
    }
}

impl Default for LinuxPlatform {
    fn default() -> Self {
        Self::new().expect("Failed to create Linux platform")
    }
}

impl MemoryReader for LinuxPlatform {
    fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>, PlatformError> {
        if size == 0 {
            return Ok(Vec::new());
        }

        // For self-reading (same process), we can directly read memory
        if self.pid == unsafe { libc::getpid() } {
            if !self.is_valid_address(address) {
                return Err(PlatformError::InvalidAddress(address));
            }

            // Safe because we've validated the address
            let slice = unsafe { std::slice::from_raw_parts(address as *const u8, size) };
            return Ok(slice.to_vec());
        }

        // For other processes, we need to use process_vm_readv or /proc/[pid]/mem
        // Read from /proc/[pid]/mem
        use std::io::{Seek, Read, SeekFrom};

        let mem_path = format!("/proc/{}/mem", self.pid);
        let mut file = File::open(&mem_path)
            .map_err(|e| PlatformError::Other(format!("Failed to open {}: {}", mem_path, e)))?;

        file.seek(SeekFrom::Start(address as u64))
            .map_err(|e| PlatformError::MemoryReadFailed {
                address,
                size,
                reason: format!("Seek failed: {}", e),
            })?;

        let mut buffer = vec![0u8; size];
        file.read_exact(&mut buffer)
            .map_err(|e| PlatformError::MemoryReadFailed {
                address,
                size,
                reason: format!("Read failed: {}", e),
            })?;

        Ok(buffer)
    }

    fn is_valid_address(&self, address: usize) -> bool {
        if address == 0 {
            return false;
        }

        // Check if address is in any valid region
        if let Ok(regions) = self.get_regions_cached() {
            regions.iter().any(|r| r.contains(address) && r.readable)
        } else {
            false
        }
    }
}

impl Platform for LinuxPlatform {
    fn get_modules(&self) -> Result<Vec<ModuleInfo>, PlatformError> {
        let maps_path = format!("/proc/{}/maps", self.pid);
        let file = File::open(&maps_path)
            .map_err(|e| PlatformError::Other(format!("Failed to open {}: {}", maps_path, e)))?;

        let reader = BufReader::new(file);
        let mut modules = Vec::new();
        let mut seen_paths = std::collections::HashSet::new();

        for line in reader.lines() {
            let line = line.map_err(|e| PlatformError::Other(format!("Failed to read maps: {}", e)))?;

            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() < 6 {
                continue;
            }

            let address_range = parts[0];
            let perms = parts[1];
            let path = parts[5];

            // Skip non-executable regions or anonymous mappings
            if !perms.contains('x') || path.starts_with('[') {
                continue;
            }

            // Skip duplicates
            if !seen_paths.insert(path.to_string()) {
                continue;
            }

            // Parse address range
            let addresses: Vec<&str> = address_range.split('-').collect();
            if addresses.len() != 2 {
                continue;
            }

            let start = usize::from_str_radix(addresses[0], 16).unwrap_or(0);

            // Find the end of this module (last contiguous mapping)
            let mut end = usize::from_str_radix(addresses[1], 16).unwrap_or(0);

            // The size calculation would need to scan all mappings for this file
            // For now, just use the first mapping
            let size = end - start;

            let name = std::path::Path::new(path)
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or("")
                .to_string();

            modules.push(ModuleInfo {
                base_address: start,
                size,
                name,
                path: path.to_string(),
            });
        }

        Ok(modules)
    }

    fn get_memory_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        self.parse_maps()
    }

    fn find_pattern(
        &self,
        pattern: &[u8],
        mask: &str,
        start: usize,
        end: usize,
    ) -> Result<Vec<PatternMatch>, PlatformError> {
        if pattern.len() != mask.len() {
            return Err(PlatformError::Other(
                "Pattern and mask length mismatch".to_string(),
            ));
        }

        if start >= end {
            return Ok(Vec::new());
        }

        let chunk_size = 0x10000; // 64KB chunks
        let mut results = Vec::new();
        let mut current = start;

        while current < end {
            let remaining = end - current;
            let read_size = remaining.min(chunk_size + pattern.len() - 1);

            // Try to read chunk
            let chunk = match self.read_bytes(current, read_size) {
                Ok(data) => data,
                Err(_) => {
                    // Skip invalid regions
                    current += chunk_size;
                    continue;
                }
            };

            // Search for pattern in chunk
            for i in 0..=(chunk.len().saturating_sub(pattern.len())) {
                if pattern_matches(&chunk[i..], pattern, mask) {
                    results.push(PatternMatch {
                        address: current + i,
                        offset: current + i - start,
                    });
                }
            }

            current += chunk_size;
        }

        Ok(results)
    }
}

// Linux platform is Send + Sync
unsafe impl Send for LinuxPlatform {}
unsafe impl Sync for LinuxPlatform {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_platform() {
        let platform = LinuxPlatform::new();
        assert!(platform.is_ok());
    }

    #[test]
    fn test_parse_maps() {
        let platform = LinuxPlatform::new().unwrap();
        let regions = platform.parse_maps().unwrap();

        assert!(!regions.is_empty());

        // Check that we have some executable regions
        let has_exec = regions.iter().any(|r| r.executable);
        assert!(has_exec);
    }

    #[test]
    fn test_read_memory() {
        let platform = LinuxPlatform::new().unwrap();

        // Read from our own memory
        let test_value: u64 = 0x1234567890ABCDEF;
        let address = &test_value as *const u64 as usize;

        let result: u64 = platform.read(address).unwrap();
        assert_eq!(result, test_value);
    }

    #[test]
    fn test_get_modules() {
        let platform = LinuxPlatform::new().unwrap();
        let modules = platform.get_modules().unwrap();

        assert!(!modules.is_empty());

        // Should find at least one shared library
        let has_lib = modules.iter().any(|m| m.name.contains(".so"));
        assert!(has_lib);
    }
}
