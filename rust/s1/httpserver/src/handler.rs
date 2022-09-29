use http::httprequest::HttpRequest;
use http::httpresponse::{self, HttpResponse};
pub fn not_found_handle(_: &HttpRequest) -> HttpResponse {
    HttpResponse::new(
        httpresponse::StatusCode::NotFound,
        None,
        Some("404".to_string()),
    )
}
