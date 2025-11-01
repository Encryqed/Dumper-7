//! Manager subsystems
//!
//! Managers organize and cache metadata about Unreal Engine structures,
//! enums, packages, and handle name collisions.

// TODO: Day 8-12 implementation

/// Package organization
pub mod package;

/// Struct metadata caching
pub mod structs;

/// Enum management
pub mod enums;

/// Member collision handling
pub mod members;

/// Dependency graph management
pub mod dependencies;

/// Collision detection and resolution
pub mod collisions;
