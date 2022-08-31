use std::time::{Duration, Instant};

pub struct Timer {
    pub last_instant: Instant,
    pub delta: Duration,
    pub period: Duration,
    pub countdown: Duration,
    pub ready: bool,
}

impl Timer {
    pub fn new() -> Self {
        let now = Instant::now();
        Self {
            last_instant: now,
            // 间隔时间
            delta: Duration::default(),
            // 统计周期
            period: Duration::from_millis(1000),
            // 统计倒计时
            countdown: Duration::default(),
            ready: true,
        }
    }
    pub fn update(&mut self) {
        let now = Instant::now();
        self.delta = now - self.last_instant;

        // 1s统计一次 delta大于1s就说明到了统计周期 然后ready=true 这里因为一开始是默认的 所以会开始就统计一次
        self.countdown = self.countdown.checked_sub(self.delta).unwrap_or_else(|| {
            self.ready = true;
            self.last_instant = now;
            self.period
        });
    }
}

#[cfg(test)]
mod tests {
    use std::time::Duration;
    #[test]
    fn test_check_sub() {
        // 0-0不是none
        assert_eq!(
            Duration::default().checked_sub(Duration::default()),
            Some(Duration::default())
        );
        let orgin = Duration::new(10, 9);
        let result = orgin
            .checked_sub(Duration::new(1, 2))
            .unwrap_or_else(|| Duration::default());

        assert_eq!(
            orgin.checked_sub(Duration::new(1, 2)),
            Some(Duration::new(9, 7))
        );

        assert_eq!(result, Duration::new(9, 7));
    }
    #[test]
    fn test_vec_from() {
        let mut v = [0; 6];
        println!("{:?}", v);

        let v2 = Vec::from(&v[..6]);
        println!("{:?}", v2);
        v[1] = 1;
        println!("{:?}", v);
        println!("{:?}", v2);
    }
}
