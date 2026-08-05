//! Shared test helpers: an in-memory capture sink and a lock that serializes
//! tests mutating `alog`'s global configuration within a single test binary.

use std::io::{self, Write};
use std::sync::{Arc, Mutex, MutexGuard};

static TEST_LOCK: Mutex<()> = Mutex::new(());

/// Acquires the process-wide test lock. Hold the returned guard for the
/// duration of any test that calls `alog::configure`/`adjust_levels`, since
/// those mutate state shared by every test in this binary.
pub fn test_lock() -> MutexGuard<'static, ()> {
    match TEST_LOCK.lock() {
        Ok(guard) => guard,
        Err(poisoned) => poisoned.into_inner(),
    }
}

/// An in-memory sink that can be handed to `alog::configure` as a
/// `Writer::Custom` and inspected afterwards via `contents()`/`lines()`.
#[derive(Clone, Default)]
pub struct CaptureSink(Arc<Mutex<Vec<u8>>>);

impl CaptureSink {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn contents(&self) -> String {
        String::from_utf8(self.0.lock().unwrap().clone()).expect("capture sink was not valid utf8")
    }

    #[allow(dead_code)]
    pub fn lines(&self) -> Vec<String> {
        self.contents().lines().map(|s| s.to_string()).collect()
    }
}

impl Write for CaptureSink {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.0.lock().unwrap().extend_from_slice(buf);
        Ok(buf.len())
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}
