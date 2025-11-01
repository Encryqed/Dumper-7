//! Platform abstraction layer
//!
//! This module provides platform-independent interfaces for:
//! - Memory reading from target processes
//! - Module enumeration
//! - Address validation
//! - Pattern scanning

use std::fmt;

#[cfg(windows)]
mod windows;
#[cfg(windows)]
pub use windows::WindowsPlatform;

#[cfg(unix)]
mod linux;
#[cfg(unix)]
pub use linux::LinuxPlatform;

#[cfg(test)]
pub mod mock;

/// Platform-specific errors
#[derive(Debug, thiserror::Error)]
pub enum PlatformError {
    /// Failed to read memory at address
    #[error("Memory read failed at 0x{address:X} (size: {size}): {reason}")]
    MemoryReadFailed {
        address: usize,
        size: usize,
        reason: String,
    },

    /// Invalid address
    #[error("Invalid memory address: 0x{0:X}")]
    InvalidAddress(usize),

    /// Module not found
    #[error("Module not found: {0}")]
    ModuleNotFound(String),

    /// Access denied
    #[error("Access denied: {0}")]
    AccessDenied(String),

    /// Platform-specific error
    #[error("Platform error: {0}")]
    Other(String),
}

/// Information about a loaded module
#[derive(Debug, Clone)]
pub struct ModuleInfo {
    /// Base address of the module
    pub base_address: usize,
    /// Size of the module in bytes
    pub size: usize,
    /// Name of the module
    pub name: String,
    /// Full path to the module
    pub path: String,
}

/// Memory region information
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MemoryRegion {
    /// Start address of the region
    pub start: usize,
    /// End address of the region (exclusive)
    pub end: usize,
    /// Whether the region is readable
    pub readable: bool,
    /// Whether the region is writable
    pub writable: bool,
    /// Whether the region is executable
    pub executable: bool,
}

impl MemoryRegion {
    /// Get the size of this memory region
    #[inline]
    pub fn size(&self) -> usize {
        self.end.saturating_sub(self.start)
    }

    /// Check if an address is within this region
    #[inline]
    pub fn contains(&self, address: usize) -> bool {
        address >= self.start && address < self.end
    }
}

/// Pattern matching result
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PatternMatch {
    /// Address where the pattern was found
    pub address: usize,
    /// Offset within the searched region
    pub offset: usize,
}

/// Core trait for reading memory from a target process
///
/// This trait abstracts platform-specific memory reading operations,
/// allowing the rest of the codebase to work with any implementation.
pub trait MemoryReader: Send + Sync {
    /// Read raw bytes from memory
    ///
    /// # Arguments
    /// * `address` - Address to read from
    /// * `size` - Number of bytes to read
    ///
    /// # Returns
    /// Vector of bytes read from memory
    fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>, PlatformError>;

    /// Read a POD (Plain Old Data) type from memory
    ///
    /// # Safety
    /// The type T must be safely transmutable from raw bytes.
    /// Use `bytemuck::Pod` to ensure this.
    fn read<T: bytemuck::Pod>(&self, address: usize) -> Result<T, PlatformError> {
        let size = std::mem::size_of::<T>();
        let bytes = self.read_bytes(address, size)?;

        if bytes.len() != size {
            return Err(PlatformError::MemoryReadFailed {
                address,
                size,
                reason: format!("Expected {} bytes, got {}", size, bytes.len()),
            });
        }

        Ok(bytemuck::pod_read_unaligned(&bytes))
    }

    /// Read a null-terminated string from memory
    fn read_cstring(&self, address: usize, max_length: usize) -> Result<String, PlatformError> {
        let bytes = self.read_bytes(address, max_length)?;

        // Find null terminator
        let null_pos = bytes.iter().position(|&b| b == 0).unwrap_or(bytes.len());

        String::from_utf8(bytes[..null_pos].to_vec())
            .map_err(|e| PlatformError::Other(format!("Invalid UTF-8 string: {}", e)))
    }

    /// Read a null-terminated wide string from memory
    fn read_wstring(&self, address: usize, max_length: usize) -> Result<String, PlatformError> {
        let byte_count = max_length * 2;
        let bytes = self.read_bytes(address, byte_count)?;

        // Convert to u16 array
        let wide: Vec<u16> = bytes
            .chunks_exact(2)
            .map(|chunk| u16::from_le_bytes([chunk[0], chunk[1]]))
            .take_while(|&c| c != 0)
            .collect();

        String::from_utf16(&wide)
            .map_err(|e| PlatformError::Other(format!("Invalid UTF-16 string: {}", e)))
    }

    /// Check if an address is valid (readable)
    fn is_valid_address(&self, address: usize) -> bool;

    /// Check if a memory range is valid
    fn is_valid_range(&self, address: usize, size: usize) -> bool {
        // Check for overflow
        if address.checked_add(size).is_none() {
            return false;
        }

        // For small ranges, just check the start and end
        if size <= 0x1000 {
            return self.is_valid_address(address)
                && self.is_valid_address(address + size - 1);
        }

        // For larger ranges, we'd need to check memory regions
        // Implementation can be optimized per-platform
        self.is_valid_address(address)
    }
}

/// Extended platform operations
pub trait Platform: MemoryReader {
    /// Get all loaded modules in the target process
    fn get_modules(&self) -> Result<Vec<ModuleInfo>, PlatformError>;

    /// Get a specific module by name
    fn get_module(&self, name: &str) -> Result<ModuleInfo, PlatformError> {
        self.get_modules()?
            .into_iter()
            .find(|m| m.name.eq_ignore_ascii_case(name))
            .ok_or_else(|| PlatformError::ModuleNotFound(name.to_string()))
    }

    /// Get all memory regions
    fn get_memory_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError>;

    /// Get executable memory regions (for pattern scanning)
    fn get_executable_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        Ok(self.get_memory_regions()?
            .into_iter()
            .filter(|r| r.executable)
            .collect())
    }

    /// Find a pattern in memory
    ///
    /// # Arguments
    /// * `pattern` - Byte pattern to search for
    /// * `mask` - Mask string where 'x' means match, '?' means wildcard
    /// * `start` - Start address
    /// * `end` - End address
    ///
    /// # Returns
    /// Vector of all matches found
    fn find_pattern(
        &self,
        pattern: &[u8],
        mask: &str,
        start: usize,
        end: usize,
    ) -> Result<Vec<PatternMatch>, PlatformError>;

    /// Find the first occurrence of a pattern
    fn find_pattern_first(
        &self,
        pattern: &[u8],
        mask: &str,
        start: usize,
        end: usize,
    ) -> Result<Option<PatternMatch>, PlatformError> {
        Ok(self.find_pattern(pattern, mask, start, end)?.into_iter().next())
    }

    /// Scan all executable regions for a pattern
    fn scan_pattern(
        &self,
        pattern: &[u8],
        mask: &str,
    ) -> Result<Vec<PatternMatch>, PlatformError> {
        let regions = self.get_executable_regions()?;
        let mut results = Vec::new();

        for region in regions {
            if let Ok(mut matches) = self.find_pattern(pattern, mask, region.start, region.end) {
                results.append(&mut matches);
            }
        }

        Ok(results)
    }
}

/// Helper function to check if a pattern matches at a specific position
#[inline]
pub fn pattern_matches(data: &[u8], pattern: &[u8], mask: &str) -> bool {
    if data.len() < pattern.len() || mask.len() != pattern.len() {
        return false;
    }

    for (i, (&pattern_byte, mask_char)) in pattern.iter().zip(mask.chars()).enumerate() {
        if mask_char == 'x' && data[i] != pattern_byte {
            return false;
        }
    }

    true
}

/// Default platform implementation for the current OS
#[cfg(windows)]
pub type DefaultPlatform = WindowsPlatform;

#[cfg(unix)]
pub type DefaultPlatform = LinuxPlatform;

impl fmt::Debug for dyn MemoryReader {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MemoryReader").finish_non_exhaustive()
    }
}

impl fmt::Debug for dyn Platform {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Platform").finish_non_exhaustive()
    }
}
