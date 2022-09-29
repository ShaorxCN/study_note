use crate::handler;
use http::{
    httprequest::{self, HttpRequest},
    httpresponse::HttpResponse,
};
use std::{collections::HashMap, io::Write};
trait GetHandle {
    fn get(&self, _path: &String) -> fn(&HttpRequest) -> HttpResponse {
        handler::not_found_handle
    }
}
struct UninitializedRouter;

enum GetRouter<'a> {
    NotFound(&'a UninitializedRouter),
    Found(&'a RealRouter),
}

impl<'a> GetHandle for GetRouter<'a> {
    fn get(&self, path: &String) -> fn(&HttpRequest) -> HttpResponse {
        match self {
            GetRouter::NotFound(t) => t.get(path),
            GetRouter::Found(t) => GetHandle::get(*t, path),
        }
    }
}

impl GetHandle for UninitializedRouter {}

type RealRouter = HashMap<String, fn(&HttpRequest) -> HttpResponse>;
impl GetHandle for RealRouter {
    fn get(&self, path: &String) -> fn(&HttpRequest) -> HttpResponse {
        if let Some(f) = self.get(path) {
            *f
        } else {
            handler::not_found_handle
        }
    }
}
pub struct Router {
    get_handlers: RealRouter,
    post_handlers: RealRouter,
    uninitialized: UninitializedRouter,
}

impl Router {
    pub fn new() -> Self {
        Router {
            get_handlers: HashMap::new(),
            post_handlers: HashMap::new(),
            uninitialized: UninitializedRouter,
        }
    }

    pub fn add_get_handle(&mut self, path: String, handle: fn(&HttpRequest) -> HttpResponse) {
        self.get_handlers.insert(path, handle);
    }

    pub fn add_post_handle(&mut self, path: String, handle: fn(&HttpRequest) -> HttpResponse) {
        self.post_handlers.insert(path, handle);
    }

    pub fn serve(&self, req: &HttpRequest, stream: &mut impl Write) {
        let route: GetRouter = match req.method {
            httprequest::Method::Get => GetRouter::Found(&self.get_handlers),
            httprequest::Method::Post => GetRouter::Found(&self.post_handlers),
            _ => GetRouter::NotFound(&self.uninitialized),
        };

        let _ = stream.write(String::from(route.get(&req.resource)(req)).as_bytes());
    }
}
