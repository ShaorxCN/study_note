use std::{
    collections::HashMap,
    io::{Result, Write},
};

use crate::httprequest;

#[derive(Debug, Clone, Copy, Eq, PartialEq, Hash)]
pub enum StatusCode {
    OK = 200,
    NotFound = 404,
    BadRequest = 400,
    InternalServerError = 500,
}

impl ToString for StatusCode {
    fn to_string(&self) -> String {
        match self {
            StatusCode::OK => "OK".into(),
            StatusCode::BadRequest => "Bad Request".into(),
            StatusCode::NotFound => "Not Found".into(),
            StatusCode::InternalServerError => "Internal Server Error".into(),
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub struct HttpResponse {
    version: httprequest::Version,
    status_code: StatusCode,
    status_text: String,
    headers: Option<HashMap<String, String>>,
    body: Option<String>,
}

impl Default for HttpResponse {
    fn default() -> Self {
        Self {
            version: httprequest::Version::V1_1,
            status_code: StatusCode::OK,
            status_text: "".into(),
            headers: None,
            body: None,
        }
    }
}

impl From<HttpResponse> for String {
    fn from(res: HttpResponse) -> Self {
        let c1 = res.clone();
        format!(
            "{} {:?} {}\r\n{}{}",
            &c1.version().to_string(),
            &c1.status_code(),
            &c1.status_text(),
            &c1.headers(),
            &c1.gen_body(),
        )
    }
}

impl HttpResponse {
    pub fn new(
        status_code: StatusCode,
        headers: Option<HashMap<String, String>>,
        body: Option<String>,
    ) -> HttpResponse {
        let mut response: HttpResponse = HttpResponse::default();
        if status_code != StatusCode::OK {
            response.status_code = status_code;
        }
        response.headers = match &headers {
            Some(_h) => headers,
            None => {
                let mut h = HashMap::new();
                h.insert("Content-Type".to_string(), "text/html".to_string());
                Some(h)
            }
        };
        response.status_text = status_code.to_string();

        response.body = body;

        response
    }

    fn version(&self) -> httprequest::Version {
        self.version
    }

    fn status_code(&self) -> isize {
        self.status_code as isize
    }

    fn status_text(&self) -> &String {
        &self.status_text
    }

    fn headers(&self) -> String {
        let map = self.headers.clone().unwrap();
        let mut headers_string = "".into();

        for (k, v) in map.iter() {
            headers_string = format!("{}{}:{}\r\n", headers_string, k, v);
        }

        headers_string
    }

    fn body(&self) -> &str {
        match &self.body {
            Some(b) => b.as_str(),
            None => "",
        }
    }

    fn gen_body(&self) -> String {
        let body = self.body();
        let len = body.len();
        if let 0 = len {
            "".to_string()
        } else {
            let mut res = String::from("Content-Length:").to_owned();
            res.push_str(format!("{}\r\n\r\n{}", len, body).as_str());
            res
        }
    }

    pub fn send_response(self, writer: &mut impl Write) -> Result<()> {
        let response_string = String::from(self);
        let _ = write!(writer, "{}", response_string);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_response_struct_creation_200() {
        let response_actual = HttpResponse::new(StatusCode::OK, None, Some("xxxx你好".into()));
        let response_expected = HttpResponse {
            version: httprequest::Version::V1_1,
            status_code: StatusCode::OK,
            status_text: "OK".into(),
            headers: {
                let mut h = HashMap::new();
                h.insert("Content-Type".to_string(), "text/html".to_string());
                Some(h)
            },
            body: Some("xxxx你好".into()),
        };
        assert_eq!(response_actual, response_expected);
    }

    #[test]
    fn test_http_response_creation() {
        let response_expected = HttpResponse {
            version: httprequest::Version::V1_1,
            status_code: StatusCode::NotFound,
            status_text: "Not Found".into(),
            headers: {
                let mut h = HashMap::new();
                h.insert("Content-Type".to_string(), "text/html".to_string());
                Some(h)
            },
            body: Some("xxxx你好".into()),
        };
        let http_string: String = response_expected.into();
        let actual_string =
            "HTTP/1.1 404 Not Found\r\nContent-Type:text/html\r\nContent-Length:10\r\n\r\nxxxx你好";
        assert_eq!(http_string, actual_string);
    }

    #[test]
    fn test_b() {
        let s1 = "hello world你";
        let s2 = b"hello world";

        println!("===={:?}", s1);
        println!("===={:?}", s2);
        let b = "basd哈";
        let c: i64 = 12;
        let f = String::from("basd哈");
        let e = &c;
        let d = &b;

        // let g = &b[..1];
        // let vec: Vec<i64> = vec![1, 2, 3];
        // let int_slice = &vec[..];
        println!("===={:?}", std::mem::size_of_val(b)); // 7
        println!("===={:?}", std::mem::size_of_val(e)); // 8
        println!("===={:?}", std::mem::size_of_val(d)); // 16
        println!("===={:?}", std::mem::size_of_val(&f)); // 24
        println!("===={:?}", std::mem::size_of::<*const i8>()); // 8

        let x = "你好".to_string();
        let b = "你好".to_string();

        println!("{:?}  {:?}", x, x.as_ptr());
        println!("{:?}  {:?}", b, b.as_ptr());

        println!(" {:?}", x == b);
    }
}
