//! Formatters that turn a [`LogRecord`] into one or more lines of output.

use serde_json::{json, Value};

use crate::record::LogRecord;

/// The fixed number of space characters used for a single indentation level,
/// matching the `cpp` and `python` implementations.
const INDENT: &str = "  ";

/// A formatter turns a single log record into the text that should be
/// written to the configured sink, including the trailing newline.
pub trait Formatter: Send + Sync {
    fn format(&self, record: &LogRecord<'_>) -> String;
}

/// Formats records as human-readable, aligned lines of text, intended for
/// use while actively developing.
///
/// Header format (per the implementation spec):
/// `"timestamp [channel:level(:thread_id)] (indentation)"`
#[derive(Debug, Clone)]
pub struct PrettyFormatter {
    /// The fixed width that channel names are padded/truncated to.
    pub channel_width: usize,
}

impl Default for PrettyFormatter {
    fn default() -> Self {
        Self { channel_width: 5 }
    }
}

impl PrettyFormatter {
    pub fn new(channel_width: usize) -> Self {
        Self { channel_width }
    }

    /// Builds the header for `record`, including the trailing separator
    /// space and indentation, so that message/map lines can simply be
    /// appended directly after it.
    fn header(&self, record: &LogRecord<'_>) -> String {
        let channel = pad_or_truncate(record.channel, self.channel_width);
        let mut header = format!(
            "{} [{}:{}",
            record.timestamp,
            channel,
            record.level.abbrev()
        );
        if let Some(thread_id) = record.thread_id {
            header.push(':');
            header.push_str(thread_id);
        }
        header.push_str("] ");
        header.push_str(&INDENT.repeat(record.num_indent as usize));
        header
    }
}

fn pad_or_truncate(s: &str, width: usize) -> String {
    if s.len() > width {
        s[..width].to_string()
    } else {
        format!("{:width$}", s, width = width)
    }
}

impl Formatter for PrettyFormatter {
    fn format(&self, record: &LogRecord<'_>) -> String {
        let header = self.header(record);

        let mut lines: Vec<String> = record
            .message
            .split('\n')
            .map(|line| format!("{}{}", header, line))
            .collect();

        if let Some(extra) = record.extra {
            for (key, value) in extra.iter() {
                lines.push(format!("{} * {}: {}", header, key, value));
            }
        }

        let mut out = lines.join("\n");
        out.push('\n');
        out
    }
}

/// Formats records as single-line JSON objects, intended for consumption by
/// log aggregation systems.
#[derive(Debug, Clone, Default)]
pub struct JsonFormatter;

impl Formatter for JsonFormatter {
    fn format(&self, record: &LogRecord<'_>) -> String {
        let mut map = serde_json::Map::new();
        map.insert("channel".to_string(), json!(record.channel));
        map.insert("level".to_string(), json!(record.level().ordinal()));
        map.insert("level_str".to_string(), json!(record.level_str()));
        map.insert("timestamp".to_string(), json!(record.timestamp));
        map.insert("message".to_string(), json!(record.message));
        map.insert("num_indent".to_string(), json!(record.num_indent));
        if let Some(thread_id) = record.thread_id {
            map.insert("thread_id".to_string(), json!(thread_id));
        }
        if let Some(extra) = record.extra {
            for (key, value) in extra.iter() {
                map.insert(key.clone(), value.clone());
            }
        }

        let mut out = Value::Object(map).to_string();
        out.push('\n');
        out
    }
}
