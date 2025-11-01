# Dumper-7 Rust Architecture

This document explains the architecture of the Rust rewrite and the rationale behind key design decisions.

## Design Principles

### 1. Dependency Injection Over Global State

**Problem in C++ version:**
```cpp
class StructManager {
    static inline HashStringTable UniqueNames;
    static inline std::unordered_map<uint64, StructInfo> Cache;

    static void AddStruct(...) {
        // Uses global state - can't test, can't reset, not thread-safe
    }
};
```

**Solution in Rust:**
```rust
pub struct StructManager {
    unique_names: Arc<StringTable>,
    cache: Arc<RwLock<HashMap<u64, StructInfo>>>,
}

impl StructManager {
    pub fn new(unique_names: Arc<StringTable>) -> Self {
        Self {
            unique_names,
            cache: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    pub fn add_struct(&self, info: StructInfo) {
        self.cache.write().insert(info.id, info);
    }
}
```

**Benefits:**
- ✅ Testable - Can create isolated instances
- ✅ Thread-safe - RwLock provides safe concurrent access
- ✅ Resettable - Can create fresh context for each generation
- ✅ Mockable - Can inject test doubles

### 2. Trait-Based Platform Abstraction

**Why traits?**

Traits allow us to write platform-independent code that works with any implementation:

```rust
fn find_offset<P: Platform>(platform: &P) -> Result<usize> {
    // Works with Windows, Linux, or Mock platform
    let pattern = &[0x48, 0x8B, 0x05];
    let matches = platform.find_pattern(pattern, "xxx", 0, usize::MAX)?;
    Ok(matches[0].address)
}
```

**Platform implementations:**

1. **WindowsPlatform** - Real implementation using Win32 API
   - `ReadProcessMemory` for reading
   - `VirtualQuery` for validation
   - `EnumProcessModules` for module enumeration

2. **LinuxPlatform** - Real implementation using Linux APIs
   - `/proc/[pid]/mem` for reading
   - `/proc/[pid]/maps` for regions
   - `ptrace` for external process access

3. **MockPlatform** - Testing implementation
   - In-memory HashMap storage
   - Predictable data for tests
   - No real process required

### 3. Error Handling Strategy

**Levels of error handling:**

```rust
// Low-level: Platform errors
#[derive(Error)]
pub enum PlatformError {
    #[error("Memory read failed at 0x{address:X}")]
    MemoryReadFailed { address: usize, size: usize, reason: String },

    #[error("Invalid address: 0x{0:X}")]
    InvalidAddress(usize),
}

// Mid-level: Component errors
#[derive(Error)]
pub enum OffsetError {
    #[error("Offset not found for {name}: {reason}")]
    NotFound { name: String, reason: String },

    #[error("Validation failed: {0}")]
    ValidationFailed(String),
}

// Top-level: Application errors
#[derive(Error)]
pub enum DumperError {
    #[error("Platform error: {0}")]
    Platform(#[from] PlatformError),

    #[error("Offset error: {0}")]
    Offset(#[from] OffsetError),

    #[error("Generation failed: {0}")]
    Generation(String),
}
```

**Error context:**

Using `anyhow` for adding context:

```rust
finder.find_gobjects_offset()
    .context("Failed to locate GObjects array")?;

// Error output:
// Failed to locate GObjects array
// Caused by:
//   Offset not found for GObjects: pattern not found
```

### 4. Memory Safety Guarantees

**C++ version issues:**
```cpp
uint8* ReadMemory(uintptr_t Address, size_t Size) {
    uint8* Buffer = new uint8[Size];  // Manual memory management
    ReadProcessMemory(Handle, Address, Buffer, Size, nullptr);
    return Buffer;  // Caller must delete!
}

// Somewhere else:
auto data = ReadMemory(addr, 100);
// ... forgot to delete[] -> leak!
```

**Rust version:**
```rust
fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>> {
    let mut buffer = vec![0u8; size];  // Automatically freed

    unsafe {
        ReadProcessMemory(
            self.process_handle,
            address as *const _,
            buffer.as_mut_ptr() as *mut _,
            size,
            Some(&mut bytes_read),
        )?;
    }

    Ok(buffer)  // Ownership transferred, no leaks possible
}
```

**Benefits:**
- No memory leaks - RAII and Drop automatically clean up
- No use-after-free - Borrow checker prevents it
- No data races - Sync/Send enforce thread safety

### 5. Testing Architecture

**Test pyramid:**

```
        /\
       /  \      Integration Tests
      /____\     (End-to-end SDK generation)
     /      \
    /  Unit  \   Component Tests
   /  Tests   \  (Individual generators, managers)
  /____________\
                 Platform Tests
                 (Memory reading, pattern scanning)
```

**Example test hierarchy:**

```rust
// Level 1: Platform tests (using real memory)
#[test]
fn test_read_memory() {
    let platform = WindowsPlatform::new().unwrap();
    let value = 42u64;
    let result: u64 = platform.read(&value as *const _ as usize).unwrap();
    assert_eq!(result, value);
}

// Level 2: Component tests (using mock platform)
#[test]
fn test_offset_finder() {
    let platform = MockPlatform::with_sample_uobject();
    let finder = OffsetFinder::new(platform);
    let offset = finder.find_index_offset().unwrap();
    assert_eq!(offset, 0x0C);
}

// Level 3: Integration tests (full pipeline)
#[test]
fn test_full_generation() {
    let platform = MockPlatform::with_complete_game_data();
    let generator = Generator::new(platform);
    let sdk = generator.generate().unwrap();
    assert!(sdk.classes.contains("AActor"));
}
```

## Component Architecture

### Platform Layer

```
┌─────────────────────────────────────┐
│         Trait: Platform             │
│  - read_bytes(addr, size)          │
│  - is_valid_address(addr)          │
│  - get_modules()                   │
│  - find_pattern(pattern, mask)     │
└─────────────────────────────────────┘
           ▲           ▲          ▲
           │           │          │
    ┌──────┘      ┌────┘          └─────┐
    │             │                     │
┌───┴────┐  ┌─────┴─────┐  ┌───────────┴──┐
│Windows │  │   Linux   │  │     Mock     │
│Platform│  │ Platform  │  │   Platform   │
└────────┘  └───────────┘  └──────────────┘
```

### Engine Layer

```
┌────────────────────────────────────────┐
│         Unreal Engine Types            │
├────────────────────────────────────────┤
│  FName, FString, TArray, TMap, etc.   │
└────────────────────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────┐
│       UObject Hierarchy                │
├────────────────────────────────────────┤
│ UObject → UField → UStruct → UClass   │
│                  └→ UFunction          │
│         → UProperty (all variants)     │
└────────────────────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────┐
│        Object/Name Arrays              │
├────────────────────────────────────────┤
│  GObjects (FUObjectArray)             │
│  GNames   (FNamePool)                 │
└────────────────────────────────────────┘
```

### Generator Layer

```
┌─────────────────────────────────────────┐
│       Generator Context                 │
│  (Owns all managers)                    │
├─────────────────────────────────────────┤
│ - PackageManager                        │
│ - StructManager                         │
│ - EnumManager                           │
│ - MemberManager                         │
│ - DependencyManager                     │
│ - CollisionManager                      │
└─────────────────────────────────────────┘
                   │
      ┌────────────┼────────────┐
      ▼            ▼            ▼
┌──────────┐ ┌──────────┐ ┌──────────┐
│   Cpp    │ │ Mapping  │ │   IDA    │
│Generator │ │Generator │ │Generator │
└──────────┘ └──────────┘ └──────────┘
      │
      ├─ MemberGenerator
      ├─ FunctionGenerator
      ├─ TypeResolver
      └─ FileOrchestrator
```

## Data Flow

### SDK Generation Pipeline

```
1. Initialize Platform
   └─> WindowsPlatform::new()

2. Find Offsets
   └─> OffsetFinder::find_all()
       ├─> find_gobjects_offset()
       ├─> find_gnames_offset()
       └─> find_processevent_offset()

3. Initialize Engine
   └─> ObjectArray::new(gobjects_offset)
   └─> NameArray::new(gnames_offset)

4. Create Generator Context
   └─> GeneratorContext::new()
       ├─> PackageManager::new()
       ├─> StructManager::new()
       └─> ... other managers

5. Discover Objects
   └─> for object in object_array.iter()
       ├─> Classify object (Class, Struct, Enum, Function)
       └─> Add to appropriate manager

6. Resolve Dependencies
   └─> DependencyManager::resolve_all()
       └─> Topological sort for includes

7. Generate Code
   └─> CppGenerator::generate()
       ├─> For each package:
       │   ├─> Generate header
       │   ├─> Generate classes
       │   └─> Generate functions
       └─> Write to disk

8. Generate Mappings
   └─> MappingGenerator::generate()
       └─> JSON with name→address mappings
```

## Performance Considerations

### Memory Validity Caching

**Problem:** Checking every address with VirtualQuery is slow.

**Solution:** Cache memory regions for 1 second:

```rust
struct MemoryValidityCache {
    regions: Vec<MemoryRegion>,
    last_update: Instant,
}

impl WindowsPlatform {
    fn is_valid_address(&self, address: usize) -> bool {
        // Update cache if stale
        if self.cache.read().should_update() {
            self.update_cache();
        }

        // Fast lookup in cached regions
        self.cache.read().contains(address)
    }
}
```

### Pattern Scanning Optimization

**Chunked reading:**
```rust
const CHUNK_SIZE: usize = 0x10000; // 64KB

fn find_pattern(&self, pattern: &[u8], ...) -> Result<Vec<PatternMatch>> {
    let mut current = start;

    while current < end {
        // Read chunk + overlap for pattern spanning boundaries
        let size = CHUNK_SIZE + pattern.len() - 1;
        let chunk = self.read_bytes(current, size)?;

        // Search in chunk
        for i in 0..=(chunk.len() - pattern.len()) {
            if matches(&chunk[i..], pattern, mask) {
                results.push(current + i);
            }
        }

        current += CHUNK_SIZE;  // Don't double-count overlap
    }
}
```

### String Deduplication

**Problem:** SDK has millions of duplicate strings (type names, "class ", etc.)

**Solution:** Hash-based string table:

```rust
pub struct StringTable {
    strings: RwLock<HashMap<u64, String>>,
}

impl StringTable {
    pub fn intern(&self, s: &str) -> u64 {
        let hash = hash(s);

        let read_guard = self.strings.read();
        if read_guard.contains_key(&hash) {
            return hash;  // Already interned
        }
        drop(read_guard);

        self.strings.write().insert(hash, s.to_string());
        hash
    }

    pub fn get(&self, hash: u64) -> Option<String> {
        self.strings.read().get(&hash).cloned()
    }
}
```

## Thread Safety

All shared state uses appropriate synchronization:

```rust
// Read-heavy: RwLock
pub struct StructManager {
    cache: Arc<RwLock<HashMap<u64, StructInfo>>>,  // Many readers, rare writers
}

// Write-heavy: Mutex
pub struct Logger {
    output: Arc<Mutex<File>>,  // Serialize all writes
}

// Read-only: Arc alone
pub struct Config {
    settings: Arc<Settings>,  // Immutable after creation
}
```

## Future Extensibility

### Adding New Platforms

```rust
// Example: Add macOS support
pub struct MacOSPlatform {
    task_port: mach_port_t,
}

impl MemoryReader for MacOSPlatform {
    fn read_bytes(&self, address: usize, size: usize) -> Result<Vec<u8>> {
        // Use mach_vm_read
    }
}

impl Platform for MacOSPlatform {
    // Implement other methods
}
```

### Adding New Generators

```rust
// Example: Add Python bindings generator
pub struct PythonGenerator {
    context: Arc<GeneratorContext>,
}

impl Generator for PythonGenerator {
    fn generate(&self, output_path: &Path) -> Result<()> {
        // Generate .py files with ctypes bindings
    }
}
```

### Plugin System

Future enhancement - dynamic generator loading:

```rust
#[no_mangle]
pub extern "C" fn create_generator() -> Box<dyn Generator> {
    Box::new(CustomGenerator::new())
}
```

## Comparison with C++ Version

| Aspect | C++ (Before) | Rust (After) |
|--------|-------------|--------------|
| **Architecture** | Static global state | Dependency injection |
| **Testing** | Not testable | Comprehensive tests |
| **Concurrency** | Not thread-safe | Thread-safe by default |
| **Memory Safety** | Manual, error-prone | Compile-time guaranteed |
| **Error Handling** | cerr + exceptions | Result<T, E> |
| **Platform Support** | Windows only | Windows + Linux |
| **Code Organization** | 8K line files | <1K line modules |
| **Dependencies** | None (embedded) | Managed by Cargo |
| **Build Time** | Fast | Slow first build, fast incremental |
| **Runtime Performance** | Excellent | Same or better |

## Summary

The Rust rewrite maintains all the sophisticated algorithms of the original Dumper-7 while addressing fundamental architectural issues:

1. **No global state** - Everything uses dependency injection
2. **Fully testable** - Mock implementations enable unit tests
3. **Cross-platform** - Abstract platform layer
4. **Type-safe** - Rust's type system prevents bugs
5. **Maintainable** - Small, focused modules

The result is a codebase that's easier to understand, modify, and extend while maintaining the power and performance of the original.
