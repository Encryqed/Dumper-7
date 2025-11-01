//! Code generation subsystems
//!
//! Generators produce various output formats from the discovered
//! Unreal Engine structures.

// TODO: Day 13-18 implementation

/// C++ SDK generation
pub mod cpp;

/// Mapping file generation
pub mod mapping;

/// IDA mapping generation
pub mod ida;

/// Dumpspace format generation
pub mod dumpspace;
