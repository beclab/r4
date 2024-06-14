pub const MONGO_ID: &str = "id";
pub const MONGO_FEED_ID: &str = "feed";
pub const MONGO_STATUS: &str = "status";
pub const MONGO_TITLE: &str = "title";
pub const MONGO_URL: &str = "url";
pub const MONGO_FULL_CONTENT: &str = "full_content";
pub const MONGO_RAW_CONTENT: &str = "raw_content";

#[derive(Default, Clone)]
pub struct Entry {
    pub id: String,
    pub feed_id: Option<String>,
    pub status: Option<String>,
    pub titile: Option<String>,
    pub url: Option<String>,
    pub full_content: Option<String>,
    pub raw_content: Option<String>,
}

impl Entry {
    pub fn new() -> Self {
        Default::default()
    }
}
