use std::{error::Error, fs::File, io, io::ErrorKind, io::Read};

fn main() -> Result<(), Box<dyn Error>> {
    // trait类似 interface{}上面可以定义方法集  然后Box可以用来做trait的返回值 他代表一个堆上数据的引用 因为指针或者引用长度是固定的
    // dyn trait 则是指明特征类型 这个dyn就是个语法 或者更加可见
    // let v = vec![1, 3, 4];
    // getV(&v)
    // file_test();
    let name = String::from("username.txt");
    // file_test_2();
    let _ = match read_username_from_file(&name) {
        Ok(uname) => println!("username : {}", uname),
        Err(e) => panic!("{:?}", e),
    };

    let f = File::open("asd.txt")?;
    Ok(())
    // let v = vec![9, 4, 2, 10, 7, 8, 8, 1, 9];
    // let ans = max_turbulence_size(v);
    // The right answer is 5. But when build with release, the output is 2
    // println!("{}", ans);
}

fn getV(v: &Vec<i32>) {
    println!("{}", v[99]);
}

fn file_test() {
    let f = File::open("./hello.txt");

    let f = match f {
        Ok(file) => {
            println!("open success");
            file
        }
        Err(error) => match error.kind() {
            ErrorKind::NotFound => match File::create("hello.txt") {
                Ok(fc) => fc,
                Err(e) => panic!("Tried to create file but there was a problem:{:?}", e),
            },
            other_error => panic!("There was a problem opening the file:{:?}", other_error),
        },
    };
}

fn max_turbulence_size(arr: Vec<i32>) -> i32 {
    let mut prev = 0;
    let mut cnt = 0;
    let mut ret = 0;
    for w in arr.windows(2) {
        let d = w[1] - w[0];
        if d == 0 {
            cnt = 0;
        } else if d > 0 {
            if prev < 0 {
                cnt += 1;
            } else {
                cnt = 1;
            }
        } else {
            if prev > 0 {
                cnt += 1;
            } else {
                cnt = 1;
            }
        }
        // Uncomment the follow line will give the right answer
        // println!("{} {}", d, prev);
        ret = ret.max(cnt);
        // The follow line seems optimized out if we don't access `prev`, `d` simultaneously.
        prev = d;
    }
    ret + 1
}

fn file_test_2() {
    // let f = File::open("hello2.txt").unwrap();
    let f = File::open("hello2.txt").expect("hah panic");
}

fn read_username_from_file(name: &String) -> Result<String, io::Error> {
    let f = File::open(name);

    let mut f = match f {
        Ok(file) => file,
        Err(e) => return Err(e),
    };

    let mut s = String::new();

    match f.read_to_string(&mut s) {
        Ok(_) => Ok(s),
        Err(e) => Err(e),
    }
}

pub struct Guess {
    value: i32,
}

impl Guess {
    pub fn new(value: i32) -> Guess {
        if value < 1 || value > 100 {
            panic!("Guess value must be between 1 and 100,got {}.", value);
        }

        Guess { value }
    }

    pub fn value(&self) -> i32 {
        self.value
    }
}
