mod handler;
mod router;
mod server;

use http::httprequest::HttpRequest;
use http::httpresponse::{self, HttpResponse};

use router::Router;
use server::Server;

fn test_get_handle(_: &HttpRequest) -> HttpResponse {
    HttpResponse::new(
        httpresponse::StatusCode::OK,
        None,
        Some(String::from("this is test")),
    )
}
fn test_post_handle(req: &HttpRequest) -> HttpResponse {
    HttpResponse::new(
        httpresponse::StatusCode::OK,
        None,
        Some(req.msg_body.clone()),
    )
}

fn main() {
    let mut r = Router::new();

    r.add_get_handle("/test".into(), test_get_handle);
    r.add_post_handle("/post".into(), test_post_handle);
    let s = Server::new("127.0.0.1:3000", r);
    s.run();
}
