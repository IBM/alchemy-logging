//! `alog` is the Rust implementation of [Alchemy
//! Logging](https://github.com/IBM/alchemy-logging), a structured logging
//! framework with independently filterable channels and fine-grained levels,
//! implemented consistently across many languages.
//!
//! ```
//! use alog::{alog, MessageLevel};
//!
//! alog::configure(alog::Config::default());
//! alog!("TEST", MessageLevel::Info, "Hello, {}!", "world");
//! ```
//!
//! Enabling the `disable-logging` feature strips every logging macro down to
//! a no-op at compile time — channel, level, and message arguments are
//! discarded unevaluated, so logging has zero footprint in the compiled
//! binary. This is the Rust analog of `cpp`'s `-D ALOG_DISABLE_LOGGING`. It
//! only affects the macros below; directly constructing a [`ScopedLog`],
//! [`ScopedTimer`], [`ScopedIndent`], or [`ScopedMetadata`] (e.g. to hold a
//! named [`ScopedTimer`] and query its elapsed time mid-scope) still works
//! and still logs normally, since that form can't be erased without leaving
//! a dangling reference to the binding.

mod core;
pub mod formatter;
pub mod level;
pub mod record;
mod scope;

pub use crate::core::{
    __log_impl, adjust_levels, configure, is_enabled, Config, Filters, FormatterKind, Writer,
};
pub use crate::formatter::{Formatter, JsonFormatter, PrettyFormatter};
pub use crate::level::{Level, MessageLevel, ParseLevelError};
pub use crate::record::{LogRecord, MapData};
pub use crate::scope::{ScopedIndent, ScopedLog, ScopedMetadata, ScopedTimer};

/// Creates a single log record on `channel` at `level` with a message built
/// from `format!`-style arguments. The arguments are only evaluated if
/// `channel`/`level` is enabled under the current configuration.
///
/// ```
/// use alog::{alog, MessageLevel};
/// alog!("TEST", MessageLevel::Info, "the value is {}", 42);
/// ```
///
/// A [`MessageLevel`] must be given — [`Level::Off`] has no `MessageLevel`
/// counterpart, so attempting to log "at" it fails to compile rather than
/// silently doing nothing at runtime:
///
/// ```compile_fail
/// use alog::{alog, Level};
/// alog!("TEST", Level::Off, "unreachable");
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog {
    ($channel:expr, $level:expr, $($arg:tt)+) => {
        if $crate::is_enabled($channel, $level) {
            $crate::__log_impl($channel, $level, format!($($arg)+), None);
        }
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog {
    ($($arg:tt)*) => {};
}

/// Like [`alog!`], but additionally attaches an arbitrary map of
/// JSON-compatible key/value pairs (a [`MapData`]) to the record. The map
/// expression and the message arguments are only evaluated if
/// `channel`/`level` is enabled.
///
/// ```
/// use alog::{alog_map, MapData, MessageLevel};
/// let mut extra = MapData::new();
/// extra.insert("request_id".to_string(), "abc-123".into());
/// alog_map!("TEST", MessageLevel::Info, extra, "handled request");
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_map {
    ($channel:expr, $level:expr, $map:expr, $($arg:tt)+) => {
        if $crate::is_enabled($channel, $level) {
            $crate::__log_impl($channel, $level, format!($($arg)+), Some($map));
        }
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_map {
    ($($arg:tt)*) => {};
}

/// Like [`alog_map!`], but uses the channel name declared by
/// [`use_channel!`] in the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_map_channel {
    ($level:expr, $map:expr, $($arg:tt)+) => {
        $crate::alog_map!(__alog_channel(), $level, $map, $($arg)+)
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_map_channel {
    ($($arg:tt)*) => {};
}

/// Returns whether `channel` is enabled at `level`, without creating a
/// record. Useful for guarding expensive multi-statement message
/// construction that doesn't fit neatly into a single `format!` call.
///
/// Always `false` when the `disable-logging` feature is enabled.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_is_enabled {
    ($channel:expr, $level:expr) => {
        $crate::is_enabled($channel, $level)
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_is_enabled {
    ($($arg:tt)*) => {
        false
    };
}

/// Like [`alog_is_enabled!`], but uses the channel name declared by
/// [`use_channel!`] in the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_is_enabled_channel {
    ($level:expr) => {
        $crate::is_enabled(__alog_channel(), $level)
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_is_enabled_channel {
    ($($arg:tt)*) => {
        false
    };
}

/// Declares a free function, `__alog_channel`, in the enclosing module which
/// returns a fixed channel name for use by [`alog_channel!`]. This is
/// intended for module-level (free-function) use, analogous to `cpp`'s
/// `ALOG_USE_CHANNEL_FREE`.
///
/// ```
/// alog::use_channel!("TEST");
///
/// fn do_thing() {
///     alog::alog_channel!(alog::MessageLevel::Debug, "doing the thing");
/// }
/// ```
#[macro_export]
macro_rules! use_channel {
    ($channel:expr) => {
        // `#[allow(dead_code)]` covers both a module that binds a channel
        // but never calls `alog_channel!`, and every module in a crate built
        // with the `disable-logging` feature, where `alog_channel!` never
        // references this function at all.
        #[inline]
        #[allow(dead_code)]
        fn __alog_channel() -> &'static str {
            $channel
        }
    };
}

/// Like [`alog!`], but uses the channel name declared by [`use_channel!`] in
/// the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_channel {
    ($level:expr, $($arg:tt)+) => {
        $crate::alog!(__alog_channel(), $level, $($arg)+)
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_channel {
    ($($arg:tt)*) => {};
}

/// Not part of the public API: computes the dot/colon-path name of the
/// enclosing function using the standard stable-Rust `fn`-pointer +
/// `type_name` trick.
#[doc(hidden)]
#[macro_export]
macro_rules! __alog_function_name {
    () => {{
        fn __alog_f() {}
        fn __alog_type_name_of<T>(_: T) -> &'static str {
            ::std::any::type_name::<T>()
        }
        let name = __alog_type_name_of(__alog_f);
        match name.strip_suffix("::__alog_f") {
            Some(stripped) => stripped,
            None => name,
        }
    }};
}

/// Function-trace convenience: creates a [`ScopedLog`] at `Trace` level on
/// `channel` whose message is the enclosing function's name, optionally
/// followed by a `format!`-style description of its arguments.
///
/// ```
/// fn do_thing() {
///     alog::alog_fn!("TEST");
///     // ... do the thing ...
/// }
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_fn {
    ($channel:expr) => {
        let _alog_fn_scope = $crate::ScopedLog::new(
            $channel,
            $crate::MessageLevel::Trace,
            format!("{}()", $crate::__alog_function_name!()),
        );
    };
    ($channel:expr, $($arg:tt)+) => {
        let _alog_fn_scope = $crate::ScopedLog::new(
            $channel,
            $crate::MessageLevel::Trace,
            format!("{}({})", $crate::__alog_function_name!(), format!($($arg)+)),
        );
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_fn {
    ($($arg:tt)*) => {};
}

/// Like [`alog_fn!`], but uses the channel name declared by
/// [`use_channel!`] in the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_fn_channel {
    () => {
        let _alog_fn_scope = $crate::ScopedLog::new(
            __alog_channel(),
            $crate::MessageLevel::Trace,
            format!("{}()", $crate::__alog_function_name!()),
        );
    };
    ($($arg:tt)+) => {
        let _alog_fn_scope = $crate::ScopedLog::new(
            __alog_channel(),
            $crate::MessageLevel::Trace,
            format!("{}({})", $crate::__alog_function_name!(), format!($($arg)+)),
        );
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_fn_channel {
    ($($arg:tt)*) => {};
}

/// Statement form of [`ScopedLog::new`] that binds the guard to a hidden
/// variable for you, analogous to `cpp`'s `ALOG_SCOPED_BLOCK`. Rust's macro
/// hygiene gives each expansion its own binding, so multiple uses in the
/// same block (or nested blocks) never collide — no C++-style unique-name
/// trick needed. If you need to control exactly when the guard drops,
/// construct a [`ScopedLog`] directly and bind it to a name instead.
///
/// ```
/// use alog::{alog_scoped_block, MessageLevel};
/// alog_scoped_block!("TEST", MessageLevel::Info, "doing work");
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_block {
    ($channel:expr, $level:expr, $($arg:tt)+) => {
        let _alog_scoped_block = $crate::ScopedLog::new($channel, $level, format!($($arg)+));
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_block {
    ($($arg:tt)*) => {};
}

/// Like [`alog_scoped_block!`], but uses the channel name declared by
/// [`use_channel!`] in the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_block_channel {
    ($level:expr, $($arg:tt)+) => {
        let _alog_scoped_block =
            $crate::ScopedLog::new(__alog_channel(), $level, format!($($arg)+));
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_block_channel {
    ($($arg:tt)*) => {};
}

/// Statement form of [`ScopedTimer::new`] that binds the guard to a hidden
/// variable for you, analogous to `cpp`'s `ALOG_SCOPED_TIMER`. If you need
/// to query the elapsed time mid-scope, construct a [`ScopedTimer`] directly
/// and bind it to a name instead (analogous to `cpp`'s
/// `ALOG_NEW_SCOPED_TIMER`) — that form can't be compiled out by the
/// `disable-logging` feature, since other code refers to it by name.
///
/// ```
/// use alog::{alog_scoped_timer, MessageLevel};
/// alog_scoped_timer!("TEST", MessageLevel::Info, "did work in ");
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_timer {
    ($channel:expr, $level:expr, $($arg:tt)+) => {
        let _alog_scoped_timer = $crate::ScopedTimer::new($channel, $level, format!($($arg)+));
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_timer {
    ($($arg:tt)*) => {};
}

/// Like [`alog_scoped_timer!`], but uses the channel name declared by
/// [`use_channel!`] in the enclosing scope instead of taking one explicitly.
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_timer_channel {
    ($level:expr, $($arg:tt)+) => {
        let _alog_scoped_timer =
            $crate::ScopedTimer::new(__alog_channel(), $level, format!($($arg)+));
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_timer_channel {
    ($($arg:tt)*) => {};
}

/// Statement form of [`ScopedIndent::new`] that binds the guard to a hidden
/// variable for you, analogous to `cpp`'s `ALOG_SCOPED_INDENT`.
///
/// ```
/// alog::alog_scoped_indent!();
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_indent {
    () => {
        let _alog_scoped_indent = $crate::ScopedIndent::new();
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_indent {
    () => {};
}

/// Statement form of [`ScopedMetadata::new`] that binds the guard to a
/// hidden variable for you, analogous to `cpp`'s `ALOG_SCOPED_METADATA`.
///
/// ```
/// use alog::{alog_scoped_metadata, MapData};
/// let mut extra = MapData::new();
/// extra.insert("request_id".to_string(), "abc-123".into());
/// alog_scoped_metadata!(extra);
/// ```
#[cfg(not(feature = "disable-logging"))]
#[macro_export]
macro_rules! alog_scoped_metadata {
    ($map:expr) => {
        let _alog_scoped_metadata = $crate::ScopedMetadata::new($map);
    };
}

#[cfg(feature = "disable-logging")]
#[macro_export]
macro_rules! alog_scoped_metadata {
    ($($arg:tt)*) => {};
}
