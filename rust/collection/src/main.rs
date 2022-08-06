use std::{collections::HashMap, hash::Hash};

fn main() {
    // print&ln!("Hello, world!");
    // let v: Vec<i32> = Vec::new();
    // println!("{:?}", v);
    // let mut a = vec![1, 2, 3];
    // let mut first = a[0];
    // println!("first element is {}", first); // 放在这边没事儿 因为这里用完就释放了 下面push只要不再用就行  简单说 不可变
    //                                         // let firstb = &mut a[0]; 声明没事儿 毕竟没用

    // first = 32;
    // println!("{}", first);
    // println!("{:?}", a);

    // a.push(11); // 可变引用
    //             // println!("{}", first); // 放在这里会出现问题 因为a.push代表可变引用 因为增加新元素导致空间不足 重新分配  但这这个输出是不可变引用

    // let mut v3 = Vec::new();
    // v3.push(4);
    // v3.push(5);
    // v3.push(6);
    // v3.push(7);
    // println!("{:?}", v3);

    // match v3.get(2) {
    //     Some(third) => println!("the third elsement is {}", third),
    //     None => println!("get none "),
    // }

    // // let mut b = vec![1, 2, 3, 4, 5];

    // // let first = &b[0];
    // // b.push(6);
    // // println!("The first element is: {}", first);

    // let mut s = String::from("asd");
    // let r1 = &s;
    // read(r1);
    // let r2 = &mut s;
    // // read(r1); //放这里不行 因为他是在声明可变引用后使用了不可变引用
    // // change(r2);
    // println!("{}", s);

    // let mut vs = vec![String::from("a"), String::from("b"), String::from("c")];
    // println!("{:?}", vs);
    // let bfirst = &vs[0]; // 这里不行 因为已经存在可变引用了
    // println!("{:?}", bfirst);
    // let mut afirst = &mut vs[0]; // 只能使用引用 因为String未实现Copy
    //                              // let mut afirst = vs.get(0); // get默认不可变引用
    //                              // match afirst {
    //                              //     Some(v) => {
    //                              //         println!("{}", v);
    //                              //         // v = String::from("aha");
    //                              //         //  v.push_str("123");
    //                              //         println!("{}", v);
    //                              //     }
    //                              //     None => println!("none"),
    //                              // }
    // println!("{}", afirst);
    // //  println!("{:?}", bfirst);  // 同理 这里不行 存在可变引用 不能使用不可变引用
    // // let bfirst = &vs[0]; // 这里不行 因为已经存在可变引用了 最近但讲 就是可变后不能使用和声明不可变
    // afirst.push_str("xyz");
    // println!("{}", afirst);
    // println!("{:?}", vs); // 会变化

    // // let mut a: i32;
    // // a = 2;
    // // a += 1;
    // // println!("{}", a);

    // //  str_test();
    hash_test2();
    count_words();
}

fn read(s: &String) {
    println!("{}", s)
}

fn change(s: &mut String) {
    s.push_str("ad")
}

fn str_test() {
    let s1 = String::from("hello,");
    let s2 = String::from("evan");

    // let s3 = s1 + &s2;  // 获取s1的所有权 将s2的内容复制到s1hou

    let s3 = format!("{}-{}", s1, s2); // 不会获取是s1的所有权
    println!("{}", s3);
    println!("{}", s1);

    let hello = "Здравствуйте";
    let length = hello.len();
    println!("{}", hello);
    println!("{}", length);
    let answer = &hello[0..4];
    println!("{}", answer);

    // for ch in hello.chars() {
    //     println!("{}", ch);
    // }

    for ch in hello.bytes() {
        println!("{}", ch);
    }
}

fn hash_test() {
    let mut scores = HashMap::new();
    let mut keya = 1;
    scores.insert(keya, 10);
    scores.insert(2, 100); // 可以直接是使用字面量key  integer 实现了copy 所以不存在所有权的转移
    keya = 3;
    //  let teams = vec!["evans", "evanshao"];
    // let teams = vec![1, 2];
    // let is = vec![10, 100];

    // zip 返回()元组
    //  scores = teams.iter().zip(is.iter()).collect(); // 迭代器返回的是引用 所以和直接插入是不一样的
    println!("{:?}", scores);

    if let Some(escore) = scores.get(&keya) {
        println!("yes  get {}", escore);
    } else {
        println!("none");
    }

    // for (key, value) in &scores {
    //     println!("{}:{}", key, value);
    // }

    let teamsi = vec![1, 2];
    let isi = vec![10, 100];

    let mut scoresi: HashMap<_, _> = teamsi.iter().zip(isi.iter()).collect();
    println!("{:?}", scoresi);

    if let Some(escorei) = scoresi.get(&1) {
        println!("{}", escorei);
    } else {
        println!("none");
    }
}

fn hash_test2() {
    //     let mut scores = HashMap::new();
    //     // scores.insert("evans", 10);
    //     // scores.insert("evanshao", 100);  // 可以直接是使用字面量key

    //     let teams = vec!["evans", "evanshao"];
    //     let is = vec![10, 100];

    //     scores = teams.iter().zip(is.iter()).collect(); // 迭代器返回的是引用

    //     println!("{:?}", scores);
    //     scores.insert(&("evans"), &(99));
    //     println!("{:?}", scores);

    let mut scores = HashMap::new();
    scores.insert(String::from("Yellow"), 10); // 所有权转移

    let key_name = String::from("Blue");

    // let res = scores.get(&key_name); // get使用引用

    scores.entry(String::from("Yellow")).or_insert(50); // 这里不需要引用  因为所有权问题 如果是key不存在 hashmap或许需要持有此处String所有权
    scores.entry(key_name).or_insert(50); // 返回可变引用

    println!("{:?}", scores);
}

fn count_words() {
    let text = "Hello world , by evan world";
    let mut map = HashMap::new();

    for word in text.split_whitespace() {
        println!("{}", word);
        // 每次新声明 且是引用
        let count = map.entry(word).or_insert(0);
        *count += 1;
    }

    println!("{:?}", map);
}
