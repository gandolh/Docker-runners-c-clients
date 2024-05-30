fn main() {
    let mut count = 0;
    let mut num = 2;

    while count < 4 {
        if is_prime(num) {
            count += 1;
        }
        num += 1;
    }

    println!("The 4th prime number is: {}", num - 1);
}

fn is_prime(n: i32) -> bool {
    if n <= 1 {
        return false;
    }
    for i in 2..n {
        if n % i == 0 {
            return false;
        }
    }
    true
}