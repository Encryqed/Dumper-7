//! Mock platform for testing
//!
//! This module provides a mock implementation of the platform traits
//! that can be used for testing without requiring real memory access.

use super::{MemoryReader, MemoryRegion, ModuleInfo, Platform, PlatformError, PatternMatch, pattern_matches};
use std::collections::HashMap;
use std::sync::Arc;
use parking_lot::RwLock;

/// Mock memory storage
#[derive(Default, Clone)]
struct MockMemory {
    /// Memory regions with their data
    regions: HashMap<usize, Vec<u8>>,
}

impl MockMemory {
    fn write(&mut self, address: usize, data: &[u8]) {
        self.regions.insert(address, data.to_vec());
    }

    fn read(&self, address: usize, size: usize) -> Option<Vec<u8>> {
        // Try exact match first
        if let Some(data) = self.regions.get(&address) {
            if data.len() >= size {
                return Some(data[..size].to_vec());
            }
        }

        // Try to find the region containing this address
        for (&base, data) in &self.regions {
            if address >= base && address < base + data.len() {
                let offset = address - base;
                let available = data.len() - offset;

                if available >= size {
                    return Some(data[offset..offset + size].to_vec());
                }
            }
        }

        None
    }

    fn is_valid(&self, address: usize) -> bool {
        if address == 0 {
            return false;
        }

        for (&base, data) in &self.regions {
            if address >= base && address < base + data.len() {
                return true;
            }
        }

        false
    }
}

/// Mock platform implementation for testing
#[derive(Clone)]
pub struct MockPlatform {
    memory: Arc<RwLock<MockMemory>>,
    modules: Arc<RwLock<Vec<ModuleInfo>>>,
    regions: Arc<RwLock<Vec<MemoryRegion>>>,
}

impl MockPlatform {
    /// Create a new mock platform
    pub fn new() -> Self {
        Self {
            memory: Arc::new(RwLock::new(MockMemory::default())),
            modules: Arc::new(RwLock::new(Vec::new())),
            regions: Arc::new(RwLock::new(Vec::new())),
        }
    }

    /// Write data to mock memory
    pub fn write_memory(&self, address: usize, data: &[u8]) {
        self.memory.write().write(address, data);

        // Update regions if needed
        let mut regions = self.regions.write();
        let end = address + data.len();

        // Check if this overlaps with existing regions
        let overlaps = regions.iter().any(|r| {
            (address >= r.start && address < r.end) || (end > r.start && end <= r.end)
        });

        if !overlaps {
            regions.push(MemoryRegion {
                start: address,
                end,
                readable: true,
                writable: true,
                executable: false,
            });
        }
    }

    /// Add a mock module
    pub fn add_module(&self, module: ModuleInfo) {
        self.modules.write().push(module);
    }

    /// Add a memory region
    pub fn add_region(&self, region: MemoryRegion) {
        self.regions.write().push(region);
    }

    /// Write a POD type to memory
    pub fn write_value<T: bytemuck::Pod>(&self, address: usize, value: &T) {
        let bytes = bytemuck::bytes_of(value);
        self.write_memory(address, bytes);
    }

    /// Create a mock platform with a sample UObject for testing
    pub fn with_sample_uobject() -> Self {
        let platform = Self::new();

        // Create a fake UObject at address 0x1000000
        // UObject structure (simplified):
        // +0x00: VTable (8 bytes)
        // +0x08: ObjectFlags (4 bytes)
        // +0x0C: InternalIndex (4 bytes)
        // +0x10: ClassPrivate (8 bytes)
        // +0x18: NamePrivate (8 bytes - FName)
        // +0x20: OuterPrivate (8 bytes)

        let uobject_addr = 0x1000000;
        let mut uobject_data = vec![0u8; 0x100];

        // VTable
        uobject_data[0..8].copy_from_slice(&0x7FFFFFFF00000000u64.to_le_bytes());

        // ObjectFlags (RF_Public | RF_Standalone)
        uobject_data[8..12].copy_from_slice(&0x00000005u32.to_le_bytes());

        // InternalIndex
        uobject_data[12..16].copy_from_slice(&0x00001234u32.to_le_bytes());

        // ClassPrivate (pointer to UClass)
        uobject_data[16..24].copy_from_slice(&0x2000000u64.to_le_bytes());

        // NamePrivate (FName with index)
        uobject_data[24..32].copy_from_slice(&0x00000042u64.to_le_bytes());

        // OuterPrivate (pointer to outer)
        uobject_data[32..40].copy_from_slice(&0x0u64.to_le_bytes());

        platform.write_memory(uobject_addr, &uobject_data);

        platform
    }
}

impl Default for MockPlatform {
    fn default() -> Self {
        Self::new()
    }
}

impl MemoryReader for MockPlatform {
    fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>, PlatformError> {
        self.memory.read().read(address, size)
            .ok_or(PlatformError::MemoryReadFailed {
                address,
                size,
                reason: "Address not found in mock memory".to_string(),
            })
    }

    fn is_valid_address(&self, address: usize) -> bool {
        self.memory.read().is_valid(address)
    }
}

impl Platform for MockPlatform {
    fn get_modules(&self) -> Result<Vec<ModuleInfo>, PlatformError> {
        Ok(self.modules.read().clone())
    }

    fn get_memory_regions(&self) -> Result<Vec<MemoryRegion>, PlatformError> {
        Ok(self.regions.read().clone())
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

        let mut results = Vec::new();

        // Search through all memory regions
        for (&base, data) in &self.memory.read().regions {
            let region_end = base + data.len();

            // Check if this region overlaps with our search range
            if region_end <= start || base >= end {
                continue;
            }

            let search_start = if base < start {
                start - base
            } else {
                0
            };

            let search_end = if region_end > end {
                end - base
            } else {
                data.len()
            };

            // Search for pattern in this region
            for i in search_start..=(search_end.saturating_sub(pattern.len())) {
                if pattern_matches(&data[i..], pattern, mask) {
                    let address = base + i;
                    results.push(PatternMatch {
                        address,
                        offset: address - start,
                    });
                }
            }
        }

        Ok(results)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_mock_platform() {
        let platform = MockPlatform::new();

        // Write some test data
        let test_data = vec![0x01, 0x02, 0x03, 0x04, 0x05];
        platform.write_memory(0x1000, &test_data);

        // Read it back
        let result = platform.read_bytes(0x1000, 5).unwrap();
        assert_eq!(result, test_data);

        // Test reading from middle
        let result = platform.read_bytes(0x1002, 2).unwrap();
        assert_eq!(result, vec![0x03, 0x04]);
    }

    #[test]
    fn test_mock_platform_write_value() {
        let platform = MockPlatform::new();

        let test_value: u64 = 0x1234567890ABCDEF;
        platform.write_value(0x2000, &test_value);

        let result: u64 = platform.read(0x2000).unwrap();
        assert_eq!(result, test_value);
    }

    #[test]
    fn test_mock_platform_pattern_search() {
        let platform = MockPlatform::new();

        let test_data = vec![
            0x00, 0x11, 0x22, 0x33,
            0xAA, 0xBB, 0xCC, 0xDD,
            0x44, 0x55, 0x66, 0x77,
        ];

        platform.write_memory(0x1000, &test_data);

        // Search for pattern
        let pattern = &[0xAA, 0xBB, 0xCC, 0xDD];
        let mask = "xxxx";

        let matches = platform.find_pattern(pattern, mask, 0x1000, 0x1100).unwrap();
        assert_eq!(matches.len(), 1);
        assert_eq!(matches[0].address, 0x1004);
    }

    #[test]
    fn test_mock_platform_with_sample_uobject() {
        let platform = MockPlatform::with_sample_uobject();

        // Read the UObject
        let vtable: u64 = platform.read(0x1000000).unwrap();
        assert_eq!(vtable, 0x7FFFFFFF00000000);

        let index: u32 = platform.read(0x100000C).unwrap();
        assert_eq!(index, 0x1234);
    }

    #[test]
    fn test_is_valid_address() {
        let platform = MockPlatform::new();
        platform.write_memory(0x1000, &[0x01, 0x02, 0x03]);

        assert!(platform.is_valid_address(0x1000));
        assert!(platform.is_valid_address(0x1001));
        assert!(platform.is_valid_address(0x1002));
        assert!(!platform.is_valid_address(0x1003));
        assert!(!platform.is_valid_address(0x2000));
        assert!(!platform.is_valid_address(0));
    }
}
