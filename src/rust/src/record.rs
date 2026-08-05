//! The log record type passed to formatters.

use crate::level::{Level, MessageLevel};

/// A JSON object used to carry arbitrary structured data attached to a log
/// record, either via `alog_map!` or via metadata scopes.
pub type MapData = serde_json::Map<String, serde_json::Value>;

/// A single log record, built by the core singleton immediately before being
/// handed to the configured [`crate::formatter::Formatter`].
///
/// This borrows from the call site rather than owning its data since it only
/// needs to live for the duration of a single `format` call.
#[derive(Debug, Clone)]
pub struct LogRecord<'a> {
    /// The channel this record was logged on.
    pub channel: &'a str,
    /// The severity level this record was logged at.
    pub level: MessageLevel,
    /// ISO 8601 formatted timestamp of when the record was created.
    pub timestamp: &'a str,
    /// The free-text message for this record.
    pub message: &'a str,
    /// The number of indentation levels active when this record was created.
    pub num_indent: u32,
    /// The id of the thread that created this record, if thread id logging
    /// is enabled.
    pub thread_id: Option<&'a str>,
    /// Arbitrary structured data attached to this record, merged from any
    /// map passed at the call site and any active metadata scopes.
    pub extra: Option<&'a MapData>,
}

impl<'a> LogRecord<'a> {
    /// The filter-facing level for this record.
    pub fn level(&self) -> Level {
        self.level.into()
    }

    /// The string label for this record's severity level.
    pub fn level_str(&self) -> &'static str {
        self.level.name()
    }
}
