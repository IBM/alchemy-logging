//! Log levels.
//!
//! Two enums are exposed rather than one: [`Level`] is filter-facing and
//! includes [`Level::Off`], while [`MessageLevel`] is what every logging
//! macro accepts and cannot represent `Off`. This makes "you can't log a
//! message at `Off`" a compile-time property instead of a runtime check.

use std::error::Error;
use std::fmt;
use std::str::FromStr;

/// A level that can be used to configure a channel's filter, including the
/// special `Off` level which disables a channel entirely.
///
/// Variants are declared in ascending order of verbosity so that the derived
/// [`Ord`] implementation can be used directly for filtering: a channel is
/// enabled for a given message level if `filter_level >= message_level`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Level {
    Off,
    Fatal,
    Error,
    Warning,
    Info,
    Trace,
    Debug,
    Debug1,
    Debug2,
    Debug3,
    Debug4,
}

/// A level that a log record may be created at. This is identical to
/// [`Level`] except that it has no `Off` variant, since it never makes sense
/// to create a log record "at" the off level.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum MessageLevel {
    Fatal,
    Error,
    Warning,
    Info,
    Trace,
    Debug,
    Debug1,
    Debug2,
    Debug3,
    Debug4,
}

impl Level {
    /// The full lowercase name of the level, used for the `level_str` record
    /// field and for parsing configuration strings.
    pub const fn name(&self) -> &'static str {
        match self {
            Level::Off => "off",
            Level::Fatal => "fatal",
            Level::Error => "error",
            Level::Warning => "warning",
            Level::Info => "info",
            Level::Trace => "trace",
            Level::Debug => "debug",
            Level::Debug1 => "debug1",
            Level::Debug2 => "debug2",
            Level::Debug3 => "debug3",
            Level::Debug4 => "debug4",
        }
    }

    /// The fixed-width (4 character) abbreviation used in the pretty header,
    /// matching the abbreviations used by the `cpp` and `python`
    /// implementations (e.g. `"INFO"`, `"DBG1"`).
    pub const fn abbrev(&self) -> &'static str {
        match self {
            Level::Off => "OFF ",
            Level::Fatal => "FATL",
            Level::Error => "ERRR",
            Level::Warning => "WARN",
            Level::Info => "INFO",
            Level::Trace => "TRCE",
            Level::Debug => "DBUG",
            Level::Debug1 => "DBG1",
            Level::Debug2 => "DBG2",
            Level::Debug3 => "DBG3",
            Level::Debug4 => "DBG4",
        }
    }

    /// The numeric enumeration value for this level, as required by the
    /// record spec's `level` field. Ascends with verbosity.
    pub const fn ordinal(&self) -> u8 {
        match self {
            Level::Off => 0,
            Level::Fatal => 1,
            Level::Error => 2,
            Level::Warning => 3,
            Level::Info => 4,
            Level::Trace => 5,
            Level::Debug => 6,
            Level::Debug1 => 7,
            Level::Debug2 => 8,
            Level::Debug3 => 9,
            Level::Debug4 => 10,
        }
    }
}

impl MessageLevel {
    pub const fn name(&self) -> &'static str {
        message_level_to_level(*self).name()
    }

    pub const fn abbrev(&self) -> &'static str {
        message_level_to_level(*self).abbrev()
    }

    pub const fn ordinal(&self) -> u8 {
        message_level_to_level(*self).ordinal()
    }
}

const fn message_level_to_level(level: MessageLevel) -> Level {
    match level {
        MessageLevel::Fatal => Level::Fatal,
        MessageLevel::Error => Level::Error,
        MessageLevel::Warning => Level::Warning,
        MessageLevel::Info => Level::Info,
        MessageLevel::Trace => Level::Trace,
        MessageLevel::Debug => Level::Debug,
        MessageLevel::Debug1 => Level::Debug1,
        MessageLevel::Debug2 => Level::Debug2,
        MessageLevel::Debug3 => Level::Debug3,
        MessageLevel::Debug4 => Level::Debug4,
    }
}

impl From<MessageLevel> for Level {
    fn from(level: MessageLevel) -> Self {
        message_level_to_level(level)
    }
}

impl Default for Level {
    /// The default filter level used when the crate has not been explicitly
    /// configured, matching every other language's implementation.
    fn default() -> Self {
        Level::Info
    }
}

impl fmt::Display for Level {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

impl fmt::Display for MessageLevel {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

/// Error returned when a string does not correspond to a known [`Level`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParseLevelError(String);

impl fmt::Display for ParseLevelError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "invalid log level: {}", self.0)
    }
}

impl Error for ParseLevelError {}

impl FromStr for Level {
    type Err = ParseLevelError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s.to_ascii_lowercase().as_str() {
            "off" => Ok(Level::Off),
            "fatal" | "critical" => Ok(Level::Fatal),
            "error" => Ok(Level::Error),
            "warning" | "warn" => Ok(Level::Warning),
            "info" => Ok(Level::Info),
            "trace" => Ok(Level::Trace),
            "debug" => Ok(Level::Debug),
            "debug1" => Ok(Level::Debug1),
            "debug2" => Ok(Level::Debug2),
            "debug3" => Ok(Level::Debug3),
            "debug4" => Ok(Level::Debug4),
            other => Err(ParseLevelError(other.to_string())),
        }
    }
}

impl FromStr for MessageLevel {
    type Err = ParseLevelError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match Level::from_str(s)? {
            Level::Off => Err(ParseLevelError(s.to_string())),
            Level::Fatal => Ok(MessageLevel::Fatal),
            Level::Error => Ok(MessageLevel::Error),
            Level::Warning => Ok(MessageLevel::Warning),
            Level::Info => Ok(MessageLevel::Info),
            Level::Trace => Ok(MessageLevel::Trace),
            Level::Debug => Ok(MessageLevel::Debug),
            Level::Debug1 => Ok(MessageLevel::Debug1),
            Level::Debug2 => Ok(MessageLevel::Debug2),
            Level::Debug3 => Ok(MessageLevel::Debug3),
            Level::Debug4 => Ok(MessageLevel::Debug4),
        }
    }
}
