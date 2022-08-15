pub struct Post {
    state: Option<Box<dyn State>>,
    content: String,
}

impl Post {
    pub fn new() -> Post {
        Post {
            state: Some(Box::new(Draft {})),
            content: String::new(),
        }
    }

    pub fn add_text(&mut self, txt: &str) {
        self.content.push_str(txt);
    }

    pub fn content(&self) -> &str {
        // as_ref 时获取Option中值得引用
        // &Option<T> to Option<&T>.
        // 这里就是&Box<dyn State>
        self.state.as_ref().unwrap().content(&self)
    }

    pub fn request_review(&mut self) {
        // take 获取Some值 留下None  这里也就时获取了Box<dyn State>得实际值
        if let Some(s) = self.state.take() {
            self.state = Some(s.request_review())
        }
    }

    pub fn reject(&mut self) {
        if let Some(s) = self.state.take() {
            self.state = Some(s.reject())
        }
    }

    pub fn approve(&mut self) {
        // take 获取Some值 留下None  这里也就时获取了Box<dyn State>得实际值
        if let Some(s) = self.state.take() {
            self.state = Some(s.approve())
        }
    }
}

// Self 指代调用者类型
trait State {
    fn request_review(self: Box<Self>) -> Box<dyn State>;
    // 这里没法使用默认 因为trait对象 不能返回Self类型
    fn reject(self: Box<Self>) -> Box<dyn State>;
    fn approve(self: Box<Self>) -> Box<dyn State>;

    // 默认实现 这样只要实现published就行
    fn content<'a>(&self, post: &'a Post) -> &'a str {
        ""
    }
}

struct Draft {}

impl State for Draft {
    // 草稿状态接收到审核请求 返回等待审批状态
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        Box::new(PendingReview {})
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        self
    }
    fn reject(self: Box<Self>) -> Box<dyn State> {
        self
    }
}

struct PendingReview {}

impl State for PendingReview {
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        self
    }

    fn reject(self: Box<Self>) -> Box<dyn State> {
        Box::new(Draft {})
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        Box::new(Published {})
    }
}

struct Published {}

impl State for Published {
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        self
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        self
    }

    fn content<'a>(&self, post: &'a Post) -> &'a str {
        &post.content
    }

    fn reject(self: Box<Self>) -> Box<dyn State> {
        self
    }
}
