use std::io::{self, Write};

fn go_collatz(mut num: u128) {
  let mut tmp: u128;
  let mut next = num + 100000000;
  num -= 1 + num % 2;
  loop {
    num += 2;
    tmp = num;
    while tmp > num {
      if (tmp >> 1) << 1 != tmp {
        tmp += (tmp >> 1) + 1;
      tmp >>= 1;
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
          _ => {},
        }
      }
    }


    match command.trim() {
      "q" => break,
      "h" => println!("q: quit, h: help, go <u128>: start from certain number, do <u128>: do certain number"),
      "help" => println!("q: quit, h: help, go <u128>: start from certain number, do <u128>: do certain number"),
      "go" => {
        println!("Usage: go <u128>");
      },
      "do" => {
        println!("Usage: do <u128>");
      },
      // print unknown command in red
      _ => println!("\x1b[31mUnknown command: {}\x1b[0m", command.trim()),
    }
  }
  return;
}