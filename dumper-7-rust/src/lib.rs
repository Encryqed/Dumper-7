//! Dumper-7: SDK Generator for Unreal Engine games
//!
//! This library provides reverse-engineering capabilities for UE4/UE5 games,
//! automatically discovering memory structures and generating complete C++ SDKs.
//!
//! # Architecture
//!
//! - **Platform Layer**: Abstracts memory reading and OS-specific operations
//! - **Engine Layer**: Understands Unreal Engine internals (UObject, FName, etc.)
//! - **Offset Finder**: Automatically discovers critical memory offsets
//! - **Managers**: Organize and cache metadata (structs, enums, packages)
//! - **Generators**: Produce output files (C++ SDK, mappings, etc.)

#![warn(missing_docs)]
#![warn(clippy::all)]
#![allow(clippy::too_many_arguments)]

pub mod platform;
pub mod engine;
pub mod offset_finder;
pub mod managers;
pub mod generators;
pub mod utils;

// Re-export commonly used types
pub use platform::{MemoryReader, Platform};

/// Result type used throughout the library
pub type Result<T, E = Error> = std::result::Result<T, E>;

/// Main error type for Dumper-7
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Platform-specific error
    #[error("Platform error: {0}")]
    Platform(#[from] platform::PlatformError),

    /// Memory reading error
    #[error("Memory read failed at address 0x{address:X}: {reason}")]
    MemoryRead {
        /// The address that failed to read
        address: usize,
        /// Why the read failed
        reason: String,
    },

    /// Offset not found
    #[error("Failed to find offset for {name}: {reason}")]
    OffsetNotFound {
        /// Name of the offset (e.g., "GObjects")
        name: String,
        /// Why it wasn't found
        reason: String,
    },

    /// Invalid offset
    #[error("Invalid offset 0x{offset:X} for {name}: {validation_error}")]
    InvalidOffset {
        /// The name of what we were looking for
        name: String,
        /// The offset that failed validation
        offset: usize,
        /// Why validation failed
        validation_error: String,
    },

    /// Code generation error
    #[error("Code generation failed: {0}")]
    Generation(String),

    /// I/O error
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    /// Serialization error
    #[error("Serialization error: {0}")]
    Serialization(#[from] serde_json::Error),
}

/// Version information
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Initialize logging for the library
pub fn init_logging() {
    use tracing_subscriber::{fmt, EnvFilter};

    fmt()
        .with_env_filter(
            EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| EnvFilter::new("dumper_7=info"))
        )
        .with_target(false)
        .with_thread_ids(true)
        .init();
}
