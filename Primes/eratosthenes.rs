use std::env;
use std::process;
use std::time::Instant;
// use std::io::{self, Write};

// Sieve of Eratosthenes
fn main () {
    let start = Instant::now();
    let args: Vec<String> = env::args().skip(1).collect();

    let max: usize = if let Some(val) = args.get(0) {
        val.parse().unwrap_or_else(|err| {
            eprintln!("Error: {}", err);
            process::exit(1);
        })
    } else {
        999999999usize // ~ 1/8 gb
    };

    let mut array = Vec::<u8>::with_capacity(max / 8 + 1);
    if array.capacity() < (max / 8 + 1) {
        eprintln!("Error: Memory allocation failed!");
        process::exit(1);
    }
    array.resize(max / 8 + 1, 0xFF);

    let mut i;
    let mut divisor = 1usize;
    let sqrt_max = (max as f64).sqrt() as usize;

    array[0] &= 0xFC; // 0 and 1 are not prime

    while divisor < sqrt_max {
        i = divisor + 1;
        while i < max { 
            if ((array[i / 8] & (1 << (i % 8)))) == 0 {
                i += 1;
                continue;
            }
            break;
        }
        divisor = i;
        // print!("> {} \r", divisor);
        // io::stdout().flush().unwrap();

        i += divisor;
        while i < max {
            array[i / 8] &= !(1 << (i % 8));
            i += divisor;
        }
    }

    let elapsed = start.elapsed();
    println!("\nSearch completed! {}ms\n", elapsed.as_millis());
    for i in 1..max {
        if ((array[i / 8] & (1 << (i % 8)))) != 0 {
            divisor = i;
            // print!("{} ", divisor);
        }
    }
    println!("Biggest Prime (< {}) is {}\n", max, divisor);
    process::exit(0);
}