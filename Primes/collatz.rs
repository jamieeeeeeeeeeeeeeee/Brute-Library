use std::io::{self, Write};
use std::time::Instant;

fn go_collatz(mut num: u128) {
  let mut tmp: u128;
  let mut bsr: u128;
  let mut next = num + 100000000;
  num -= 1 + num % 2;
  loop {
    num += 2;
    tmp = num;
    while tmp > num {
      bsr = tmp >> 1;
      if bsr << 1 != tmp {
        tmp += (bsr) + 1;
      } else {
        tmp = bsr;
      }
    }
    if num > next {
      print!("\x1b[34m{} \x1b[0m\r", num);
      io::stdout().flush().unwrap();
      next = num + 100000000;
      if num > next {
        // overflow 
        print!("\x1b\n[31mOverflow!\x1b[0m");
        break;
      }
    }
  }
}

fn time_collatz(mut range: u128) {
  let p64 = 2u64.pow(64) as u128;
  range = p64 + 10u128.pow(range as u32);
  println!("\n\x1b[34m2^64 => 2^64 + {} \x1b[0m", range);
  if range < p64 {
    println!("\x1b\n[31mOverflow!\x1b[0m");
    return;
  }
  let mut num = p64;
  let mut tmp: u128;
  let mut bsr: u128;
  num -= 1 + num % 2;
  let start = Instant::now();
  loop {
    num += 2;
    tmp = num;
    while tmp > num {
      bsr = tmp >> 1;
      if bsr << 1 != tmp {
        tmp += (bsr) + 1;
      } else {
        tmp = bsr;
      }
    }
    if num > range {
      let elapsed = start.elapsed();
      println!("\x1b[32mSearch completed in {}.{:03} seconds\x1b[0m", elapsed.as_secs(), elapsed.subsec_millis());
      return;
    }
  }
}


fn do_collatz(mut num: u128) {
    println!("{} => {} (mod 6)\n", num, num % 6);
  while num != 1 {
    if (num >> 1) << 1 != num {
      num += (num >> 1) + 1;
      println!("{} => {} (mod 6)", num, num % 6);
    } else {
      num >>= 1;
      println!("{} => {} (mod 6)", num, num % 6);
    }
  }
}

fn main() {
  loop {
    print!("\x1b[32m>> \x1b[0m");
    io::stdout().flush().unwrap();

    let mut command = String::new();
    io::stdin().read_line(&mut command).unwrap();

    if let Some((command_name, arg_str)) = command.trim().split_once(' ') {
      if let Ok(arg) = arg_str.trim().parse::<u128>() {
        match command_name {
          "go" => {
            go_collatz(arg);
            continue;
          },
          "do" => {
            do_collatz(arg);
            continue;
          },
          "perft" => {
            time_collatz(arg);
            continue;
          }
          _ => {},
        }
      }
    }


    match command.trim() {
      "q" => break,
      "h" => println!("q: quit, h: help, go <start-num>: start from certain number, do <u1specific-num28>: do certain number, perft <range-num> (from 2^4 to 2^64 + <range-num>)"),
      "help" => println!("q: quit, h: help, go <start-num>: start from certain number, do <u1specific-num28>: do certain number, perft <level-num> (from 2^644 to 2^64 + 10^<level-num>)"),
      "go" => {
        println!("Usage: go <start-num>");
      },
      "do" => {
        println!("Usage: do <specific-num>");
      },
      "perft" => {
        println!("Usage: perft <level-num>");
      }
      // print unknown command in red
      _ => println!("\x1b[31mUnknown command: {}\x1b[0m", command.trim()),
    }
  }
  return;
}