use chrono::Local;
use log::info;
use std::{error, fmt};
use std::{
    io::Write,
    time::{SystemTime, UNIX_EPOCH},
};

pub const ENTRY_COLLECTION_NAME: &str = "entries";
pub const IMPRESSIONS_COLLECTION_NAME: &str = "impressions";
pub fn init_logger() {
    let env: env_logger::Env<'_> =
        env_logger::Env::default().filter_or(env_logger::DEFAULT_FILTER_ENV, "debug");
    env_logger::Builder::from_env(env)
        .format(|buf, record| {
            writeln!(
                buf,
                "{}:{} {}  {} [{}] {}",
                record.file().unwrap_or("unknown"),
                record.line().unwrap_or(0),
                Local::now().format("%Y-%m-%d %H:%M:%S"),
                record.level(),
                record.module_path().unwrap_or("<unnamed>"),
                &record.args()
            )
        })
        .init();
    info!("env_logger initialized.");
}

pub fn join_array(current_embedding: &Vec<f32>) -> String {
    current_embedding
        .iter()
        .map(|f| f.to_string())
        .collect::<Vec<_>>()
        .join(";")
}

pub fn get_time_now() -> u64 {
    let start: SystemTime = SystemTime::now();
    let since_the_epoch: u64 = start
        .duration_since(UNIX_EPOCH)
        .expect("Time went backwards")
        .as_secs();
    return since_the_epoch;
}

#[macro_export]
macro_rules! logdetail {
    ($($args: expr),*) => {
        $(
            print!("[{}] {}:{}: {:?}\n",chrono::prelude::Local::now().format("%Y-%m-%d %H:%M:%S").to_string(), file!(), line!(),$args);
        )*
    }
}

#[derive(Debug, Clone)]
pub struct SentenceEmptyError;
impl error::Error for SentenceEmptyError {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        None
    }
}
impl fmt::Display for SentenceEmptyError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "sentence empty")
    }
}

#[cfg(test)]
mod commontest {
    use crate::common;
    use chrono::Utc;
    use text_splitter::TextSplitter;

    use super::{get_time_now, join_array};

    #[test]
    fn test_init_logger() {
        common::init_logger();
    }

    #[test]
    fn test_text_split() {
        // Maximum number of characters in a chunk
        let max_characters = 15;
        // Default implementation uses character count for chunk size
        let splitter = TextSplitter::default()
            // Optionally can also have the splitter trim whitespace for you
            .with_trim_chunks(true);

        let chunks: Vec<&str> = splitter
            .chunks("your document text", max_characters)
            .collect();
        println!("chunks {:?}", chunks);
        assert_eq!(*chunks.get(0).unwrap(), "your document");

        let chunks2: Vec<&str> = splitter.chunks("我是一个中国人,我爱我的国家", 5).collect();
        println!("chunks {:?}", chunks2);
        assert_eq!(*chunks2.get(0).unwrap(), "我是一个中");
    }

    #[test]
    fn test_grap_cuda() {
        // Grab compute code from nvidia-smi
        let mut compute_cap = {
            let out = std::process::Command::new("nvidia-smi")
                            .arg("--query-gpu=compute_cap")
                            .arg("--format=csv")
                            .output()
                            .expect("`nvidia-smi` failed. Ensure that you have CUDA installed and that `nvidia-smi` is in your PATH.");
            let out = std::str::from_utf8(&out.stdout).unwrap();
            let mut lines = out.lines();
            assert_eq!(lines.next().unwrap(), "compute_cap");
            let cap = lines.next().unwrap().replace('.', "");
            cap.parse::<usize>().unwrap()
        };

        // Grab available GPU codes from nvcc and select the highest one
        let max_nvcc_code = {
            let out = std::process::Command::new("nvcc")
                            .arg("--list-gpu-code")
                            .output()
                            .expect("`nvcc` failed. Ensure that you have CUDA installed and that `nvcc` is in your PATH.");
            let out = std::str::from_utf8(&out.stdout).unwrap();
            println!("out***** {}", out);
            let out = out.lines().collect::<Vec<&str>>();
            let mut codes = Vec::with_capacity(out.len());
            for code in out {
                let code = code.split('_').collect::<Vec<&str>>();
                if !code.is_empty() && code.contains(&"sm") {
                    if let Ok(num) = code[1].parse::<usize>() {
                        codes.push(num);
                    }
                }
            }
            codes.sort();
            if !codes.contains(&compute_cap) {
                panic!("nvcc cannot target gpu arch {compute_cap}. Available nvcc targets are {codes:?}.");
            }
            *codes.last().unwrap()
        };
    }

    #[test]
    fn test_get_time() {
        use chrono::Utc;
        let dt = Utc::now();
        println!(
            "now datetime {}",
            dt.format("%Y-%m-%dT%H:%M:%S+00:00").to_string()
        );
    }

    #[test]
    fn test_join_array() {
        let current_array: Vec<f32> = vec![1.2, 2.2, 3.1, 3.2, 4.5];
        let result: String = join_array(&current_array);
        println!("{}", result)
    }

    #[test]
    fn test_get_time_now() {
        let current_time: u64 = get_time_now();
        println!("{}", current_time)
    }

    #[test]
    fn test_compare_float() {
        let result = float_cmp::approx_eq!(f32, 0f32, 0f32, ulps = 2);
        println!("*************{}", result)
    }
}
