//! Windows platform implementation
//!
//! This module provides Windows-specific implementations for memory reading
//! and process introspection using Win32 APIs.

use super::{MemoryReader, MemoryRegion, ModuleInfo, Platform, PlatformError, PatternMatch, pattern_matches};
use std::ffi::OsString;
use std::os::windows::ffi::OsStringExt;
use std::sync::Arc;
use parking_lot::RwLock;
use windows::Win32::Foundation::{BOOL, HANDLE, HMODULE};
use windows::Win32::System::Diagnostics::Debug::{
    ReadProcessMemory, IMAGE_NT_HEADERS64, IMAGE_DOS_HEADER,
};
use windows::Win32::System::LibraryLoader::{GetModuleHandleW, GetModuleFileNameW};
use windows::Win32::System::Memory::{
    VirtualQuery, MEMORY_BASIC_INFORMATION, PAGE_EXECUTE, PAGE_EXECUTE_READ,
    PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY, PAGE_READONLY, PAGE_READWRITE,
    PAGE_WRITECOPY, MEM_COMMIT,
};
use windows::Win32::System::ProcessStatus::{
    EnumProcessModules, GetModuleInformation, MODULEINFO,
};
use windows::Win32::System::Threading::{GetCurrentProcess, OpenProcess, PROCESS_ALL_ACCESS};

/// Cache for memory validity checks
#[derive(Default)]
struct MemoryValidityCache {
    /// Cached memory regions
    regions: Vec<MemoryRegion>,
    /// Last time the cache was updated
    last_update: std::time::Instant,
}

impl MemoryValidityCache {
    const CACHE_DURATION: std::time::Duration = std::time::Duration::from_secs(1);

    fn should_update(&self) -> bool {
        self.last_update.elapsed() > Self::CACHE_DURATION
    }
}

/// Windows platform implementation
///
/// This implementation runs in-process (DLL injection) and reads from
/// the current process's memory space.
pub struct WindowsPlatform {
    /// Handle to the current process
    process_handle: HANDLE,
    /// Cached memory regions for validation
    validity_cache: Arc<RwLock<MemoryValidityCache>>,
}

impl WindowsPlatform {
    /// Create a new Windows platform instance
    ///
    /// This operates on the current process (self-reading).
    pub fn new() -> Result<Self, PlatformError> {
        unsafe {
            let process_handle = GetCurrentProcess();

            Ok(Self {
                process_handle,
                validity_cache: Arc::new(RwLock::new(MemoryValidityCache {
                    regions: Vec::new(),
                    last_update: std::time::Instant::now() - MemoryValidityCache::CACHE_DURATION,
                })),
            })
        }
    }

    /// Create a platform instance for a specific process
    pub fn attach(process_id: u32) -> Result<Self, PlatformError> {
        unsafe {
            let process_handle = OpenProcess(PROCESS_ALL_ACCESS, BOOL::from(false), process_id)
                .map_err(|e| PlatformError::AccessDenied(format!("Failed to open process {}: {}", process_id, e)))?;

            Ok(Self {
                process_handle,
                validity_cache: Arc::new(RwLock::new(MemoryValidityCache {
                    regions: Vec::new(),
                    last_update: std::time::Instant::now() - MemoryValidityCache::CACHE_DURATION,
                })),
            })
        }
    }

    /// Update the memory validity cache
    fn update_validity_cache(&self) -> Result<(), PlatformError> {
        let mut cache = self.validity_cache.write();

        if !cache.should_update() {
            return Ok(());
        }

        cache.regions = self.query_all_memory_regions()?;
        cache.last_update = std::time::Instant::now();

        Ok(())
    }

    /// Query all memory regions
    fn query_all_memory_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        let mut regions = Vec::new();
        let mut address = 0usize;

        unsafe {
            loop {
                let mut mbi = std::mem::zeroed::<MEMORY_BASIC_INFORMATION>();

                let result = VirtualQuery(
                    Some(address as *const _),
                    &mut mbi,
                    std::mem::size_of::<MEMORY_BASIC_INFORMATION>(),
                );

                if result == 0 {
                    break;
                }

                // Only include committed memory
                if mbi.State == MEM_COMMIT {
                    let protect = mbi.Protect;

                    let readable = matches!(
                        protect,
                        PAGE_READONLY
                            | PAGE_READWRITE
                            | PAGE_WRITECOPY
                            | PAGE_EXECUTE_READ
                            | PAGE_EXECUTE_READWRITE
                            | PAGE_EXECUTE_WRITECOPY
                    );

                    let writable = matches!(
                        protect,
                        PAGE_READWRITE
                            | PAGE_WRITECOPY
                            | PAGE_EXECUTE_READWRITE
                            | PAGE_EXECUTE_WRITECOPY
                    );

                    let executable = matches!(
                        protect,
                        PAGE_EXECUTE
                            | PAGE_EXECUTE_READ
                            | PAGE_EXECUTE_READWRITE
                            | PAGE_EXECUTE_WRITECOPY
                    );

                    regions.push(MemoryRegion {
                        start: mbi.BaseAddress as usize,
                        end: (mbi.BaseAddress as usize) + mbi.RegionSize,
                        readable,
                        writable,
                        executable,
                    });
                }

                // Move to next region
                address = (mbi.BaseAddress as usize) + mbi.RegionSize;

                // Prevent infinite loop on 32-bit overflow
                if address == 0 {
                    break;
                }
            }
        }

        Ok(regions)
    }

    /// Fast validity check using IsBadReadPtr-style approach
    ///
    /// This is faster than querying VirtualQuery for every check.
    #[inline]
    fn is_valid_address_fast(&self, address: usize) -> bool {
        if address == 0 || address < 0x10000 {
            return false;
        }

        // Try to read one byte - if it succeeds, the address is valid
        unsafe {
            let mut buffer = 0u8;
            let mut bytes_read = 0;

            ReadProcessMemory(
                self.process_handle,
                address as *const _,
                &mut buffer as *mut _ as *mut _,
                1,
                Some(&mut bytes_read),
            )
            .is_ok() && bytes_read == 1
        }
    }
}

impl Default for WindowsPlatform {
    fn default() -> Self {
        Self::new().expect("Failed to create Windows platform")
    }
}

impl MemoryReader for WindowsPlatform {
    fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>, PlatformError> {
        if size == 0 {
            return Ok(Vec::new());
        }

        if !self.is_valid_address(address) {
            return Err(PlatformError::InvalidAddress(address));
        }

        let mut buffer = vec![0u8; size];
        let mut bytes_read = 0;

        unsafe {
            ReadProcessMemory(
                self.process_handle,
                address as *const _,
                buffer.as_mut_ptr() as *mut _,
                size,
                Some(&mut bytes_read),
            )
            .map_err(|e| PlatformError::MemoryReadFailed {
                address,
                size,
                reason: format!("ReadProcessMemory failed: {}", e),
            })?;
        }

        if bytes_read != size {
            return Err(PlatformError::MemoryReadFailed {
                address,
                size,
                reason: format!("Expected to read {} bytes, got {}", size, bytes_read),
            });
        }

        Ok(buffer)
    }

    fn is_valid_address(&self, address: usize) -> bool {
        // Quick checks first
        if address == 0 || address < 0x10000 {
            return false;
        }

        // Use fast check
        self.is_valid_address_fast(address)
    }

    fn is_valid_range(&self, address: usize, size: usize) -> bool {
        // Check for overflow
        if address.checked_add(size).is_none() {
            return false;
        }

        // For small ranges, check endpoints
        if size <= 0x1000 {
            return self.is_valid_address(address) && self.is_valid_address(address + size - 1);
        }

        // For larger ranges, we need to check the entire range
        // Try to read the first and last pages
        self.is_valid_address(address)
            && self.is_valid_address(address + 0x1000)
            && self.is_valid_address(address + size - 1)
    }
}

impl Platform for WindowsPlatform {
    fn get_modules(&self) -> Result<Vec<ModuleInfo>, PlatformError> {
        unsafe {
            let mut modules = vec![HMODULE::default(); 1024];
            let mut bytes_needed = 0u32;

            EnumProcessModules(
                self.process_handle,
                modules.as_mut_ptr(),
                (modules.len() * std::mem::size_of::<HMODULE>()) as u32,
                &mut bytes_needed,
            )
            .map_err(|e| PlatformError::Other(format!("EnumProcessModules failed: {}", e)))?;

            let module_count = (bytes_needed as usize) / std::mem::size_of::<HMODULE>();
            modules.truncate(module_count);

            let mut result = Vec::new();

            for hmodule in modules {
                let mut module_info = std::mem::zeroed::<MODULEINFO>();

                GetModuleInformation(
                    self.process_handle,
                    hmodule,
                    &mut module_info,
                    std::mem::size_of::<MODULEINFO>() as u32,
                )
                .map_err(|e| PlatformError::Other(format!("GetModuleInformation failed: {}", e)))?;

                let mut path_buf = vec![0u16; 260];
                let path_len = GetModuleFileNameW(hmodule, &mut path_buf);

                let path = if path_len > 0 {
                    OsString::from_wide(&path_buf[..path_len as usize])
                        .to_string_lossy()
                        .into_owned()
                } else {
                    String::new()
                };

                let name = std::path::Path::new(&path)
                    .file_name()
                    .and_then(|n| n.to_str())
                    .unwrap_or("")
                    .to_string();

                result.push(ModuleInfo {
                    base_address: module_info.lpBaseOfDll as usize,
                    size: module_info.SizeOfImage as usize,
                    name,
                    path,
                });
            }

            Ok(result)
        }
    }

    fn get_memory_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        // Update cache if needed
        self.update_validity_cache()?;

        // Return cached regions
        Ok(self.validity_cache.read().regions.clone())
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

        let search_size = end - start;
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

// Implement Send + Sync for WindowsPlatform
unsafe impl Send for WindowsPlatform {}
unsafe impl Sync for WindowsPlatform {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_platform() {
        let platform = WindowsPlatform::new();
        assert!(platform.is_ok());
    }

    #[test]
    fn test_read_memory() {
        let platform = WindowsPlatform::new().unwrap();

        // Read from our own memory
        let test_value: u64 = 0x1234567890ABCDEF;
        let address = &test_value as *const u64 as usize;

        let result: u64 = platform.read(address).unwrap();
        assert_eq!(result, test_value);
    }

    #[test]
    fn test_is_valid_address() {
        let platform = WindowsPlatform::new().unwrap();

        // Test valid address (our stack)
        let valid_value = 42u32;
        assert!(platform.is_valid_address(&valid_value as *const _ as usize));

        // Test invalid addresses
        assert!(!platform.is_valid_address(0));
        assert!(!platform.is_valid_address(0x1000));
    }

    #[test]
    fn test_get_modules() {
        let platform = WindowsPlatform::new().unwrap();
        let modules = platform.get_modules().unwrap();

        assert!(!modules.is_empty());

        // Should find the current executable
        let has_exe = modules.iter().any(|m| m.name.ends_with(".exe"));
        assert!(has_exe);
    }

    #[test]
    fn test_get_memory_regions() {
        let platform = WindowsPlatform::new().unwrap();
        let regions = platform.get_memory_regions().unwrap();

        assert!(!regions.is_empty());

        // All regions should have valid ranges
        for region in regions {
            assert!(region.end > region.start);
            assert!(region.size() > 0);
        }
    }

    #[test]
    fn test_pattern_matching() {
        let platform = WindowsPlatform::new().unwrap();

        // Create a test pattern in memory
        let test_data: [u8; 16] = [
            0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD,
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        ];

        let start = test_data.as_ptr() as usize;
        let end = start + test_data.len();

        // Search for exact pattern
        let pattern = &[0xAA, 0xBB, 0xCC, 0xDD];
        let mask = "xxxx";

        let matches = platform.find_pattern(pattern, mask, start, end).unwrap();
        assert_eq!(matches.len(), 1);
        assert_eq!(matches[0].address, start + 4);

        // Search with wildcard
        let pattern = &[0xAA, 0x00, 0xCC, 0xDD];
        let mask = "x?xx";

        let matches = platform.find_pattern(pattern, mask, start, end).unwrap();
        assert_eq!(matches.len(), 1);
        assert_eq!(matches[0].address, start + 4);
    }
}
