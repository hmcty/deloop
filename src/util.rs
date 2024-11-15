use log::error;
use crate::deloop;

pub fn truncate_string(s: &str, max_len: usize) -> String {
    if s.len() > max_len {
        format!("{}...", &s[..max_len])
    } else {
        s.to_string()
    }
}

pub fn log_on_err<T>(result: Result<T, deloop::Error>) -> Result<T, deloop::Error> {
    match result {
        Ok(t) => Ok(t),
        Err(e) => {
            // TODO(hmcty): Use modal dialog
            error!("{}", e);
            Err(e)
        }
    }
}
