pub const MONGO_ID: &str = "_id";
pub const MONGO_ENTRY_ID: &str = "entry_id";
pub const MONGO_SOURCE: &str = "source";
pub const MONGO_BATCH_ID: &str = "batch_id";
pub const MONGO_POSITION: &str = "position";
pub const MONGO_CLICKED: &str = "clicked";
pub const MONGO_STARED: &str = "stared";
pub const MONGO_READ_TIME: &str = "read_time";
pub const MONGO_READ_FINISH: &str = "read_finish";
pub const MONGO_ALGORITHM_EXTRA: &str = "algorithm_extra";
pub const MONGO_EMBEDDING: &str = "embedding"; //SUB FIELD MONGO_ALGORITHM_EXTRA

#[derive(Default, Clone)]
pub struct Impression {
    pub id: String,
    pub entry_id: String,
    pub source: Option<String>,
    pub batch_id: Option<String>,
    pub position: Option<String>,
    pub clicked: Option<bool>,
    pub stared: Option<bool>,
    pub read_time: Option<f64>,
    pub read_finish: Option<bool>,
    pub embedding: Option<Vec<f32>>,
}
impl Impression {
    pub fn new() -> Self {
        Default::default()
    }
}
