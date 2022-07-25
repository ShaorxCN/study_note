pub mod smy {
    pub trait Summary {
        //  fn summarize(&self) -> String;
        // 这个是有一个默认实现  那么配合TestP 仅仅需要一个空的impl 【重载】
        // fn summarize(&self) -> String {
        //     String::from("Read more")
        // }
        fn summarize_author(&self) -> String {
            format!("(Read?)")
        }

        // 可以在一个方法里调用另外一个 这样可以方便后期扩展？不过如果author没有默认实现的话 想要实现summarize 就必须先实现author
        fn summarize(&self) -> String {
            format!("(Read more from {}...)", self.summarize_author())
        }
    }

    pub struct NewsArticle {
        pub headline: String,
        pub location: String,
        pub author: String,
        pub content: String,
    }

    impl Summary for NewsArticle {
        fn summarize(&self) -> String {
            format!("{}, by {} ({})", self.headline, self.author, self.location)
        }
    }
    #[derive(Debug)]
    pub struct Tweet {
        pub username: String,
        pub content: String,
        pub reply: bool,
        pub retweet: bool,
    }

    impl Summary for Tweet {
        fn summarize(&self) -> String {
            format!("{}: {}", self.username, self.content)
        }
    }

    impl Default for Tweet {
        fn default() -> Self {
            Tweet {
                username: String::from("evan"),
                content: String::from("content"),
                reply: true,
                retweet: false,
            }
        }
    }

    pub struct TestP {
        pub content: String,
    }

    impl Summary for TestP {}
}
