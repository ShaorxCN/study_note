use std::collections::HashMap;

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum Method {
    Get,
    Post,
    Uninitialized,
}

impl From<&str> for Method {
    fn from(s: &str) -> Method {
        match s {
            "GET" => Method::Get,
            "POST" => Method::Post,
            _ => Method::Uninitialized,
        }
    }
}
#[derive(Debug, PartialEq, Clone, Copy)]
pub enum Version {
    V1_1,
    V2_0,
    Uninitialized,
}

impl From<&str> for Version {
    fn from(s: &str) -> Version {
        match s {
            "HTTP/1.1" => Version::V1_1,
            "HTTP/2.0" => Version::V2_0,
            _ => Version::Uninitialized,
        }
    }
}

impl ToString for Version {
    fn to_string(&self) -> String {
        match self {
            Version::V1_1 => "HTTP/1.1".to_string(),
            Version::V2_0 => "HTTP/2.0".to_string(),
            Version::Uninitialized => "".to_string(),
        }
    }
}

#[derive(Debug, PartialEq)]
pub struct HttpRequest {
    pub method: Method,
    pub version: Version,
    pub resource: String,
    pub headers: HashMap<String, String>,
    pub msg_body: String,
}

impl From<&str> for HttpRequest {
    fn from(s: &str) -> HttpRequest {
        let mut start = true;
        let mut have_body = false;

        let mut parsed_method = Method::Uninitialized;
        let mut parsed_version = Version::V1_1;
        let mut parsed_resource = String::from("");
        let mut parsed_headers = HashMap::new();
        let mut parsed_msg_body = "".to_string();

        for line in s.lines() {
            if line.contains("HTTP") && start {
                let (method, resource, version) = parse_req_line(line);
                parsed_method = method;
                parsed_resource = resource;
                parsed_version = version;

                start = false;
            } else if line.contains(":") && !have_body {
                let (key, value) = parse_req_header_line(line);
                parsed_headers.insert(key, value);
            } else if line.len() == 0 {
                have_body = true;
                continue;
            } else if have_body {
                parsed_msg_body = format!("{parsed_msg_body}{line}\n");
            }
        }

        if have_body {
            parsed_msg_body = parsed_msg_body[..parsed_msg_body.len() - 1].to_string();
        }

        HttpRequest {
            method: parsed_method,
            version: parsed_version,
            resource: parsed_resource,
            headers: parsed_headers,
            msg_body: parsed_msg_body,
        }
    }
}

fn parse_req_line(s: &str) -> (Method, String, Version) {
    let mut words = s.split_whitespace();
    let method = words.next().unwrap();
    let resource = words.next().unwrap();
    let version = words.next().unwrap();
    (method.into(), resource.to_string(), version.into())
}

fn parse_req_header_line(s: &str) -> (String, String) {
    let mut header_items = s.split(":");

    let mut key = String::from("");
    let mut value = String::from("");

    if let Some(k) = header_items.next() {
        key = k.to_string();
    }
    if let Some(v) = header_items.next() {
        value = v.trim_start().to_string();
    }

    (key, value)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_method_into() {
        let m: Method = "GET".into();
        assert_eq!(m, Method::Get);
    }

    #[test]
    fn test_version_into() {
        let v: Version = "HTTP/1.1".into();
        assert_eq!(v, Version::V1_1);
    }

    #[test]

    fn test_httprequest_into() {
        let req_str = r##"POST /test HTTP/1.1
Host: asdq.com
Content-Type: text/plain
Content-Length: 48

asdqwe
dasdasqwe
evan
testasdilh ][qweo1241 ]"##;

        let request: HttpRequest = req_str.into();
        println!("into result:\n {:?}\n", request);
        let mut headers_expected = HashMap::new();
        headers_expected.insert("Host".into(), "asdq.com".into());
        headers_expected.insert("Content-Type".into(), "text/plain".into());
        headers_expected.insert("Content-Length".into(), "48".into());

        assert_eq!(Method::Post, request.method);
        assert_eq!(Version::V1_1, request.version);
        assert_eq!("/test".to_string(), request.resource);
        assert_eq!(headers_expected, request.headers);
        assert_eq!(
            r#"asdqwe
dasdasqwe
evan
testasdilh ][qweo1241 ]"#,
            request.msg_body
        );
    }
    #[test]
    fn test_httprequest_into_no_body() {
        let req_str = r##"POST /test HTTP/1.1
Host: asdq.com"##;

        let request: HttpRequest = req_str.into();
        println!("into result:\n {:?}\n", request);
        let mut headers_expected = HashMap::new();
        headers_expected.insert("Host".into(), "asdq.com".into());

        assert_eq!(Method::Post, request.method);
        assert_eq!(Version::V1_1, request.version);
        assert_eq!("/test".to_string(), request.resource);
        assert_eq!(headers_expected, request.headers);
        assert_eq!("", request.msg_body);
    }
}
