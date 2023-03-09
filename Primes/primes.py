MAX = 10_000_000
SQRT_MAX = int((MAX) ** 0.5 + 1)

array = [1 for n in range(0, MAX)]
print("\033[92m\nSieve created\033[0m")
divisor = 1

array[0] = 0
array[1] = 0

while True:
    i = divisor + 1
    while i < MAX:
        if array[i] == 0:
            i += 1
            continue
        break
    divisor = i
    print(f"{divisor}", end="\r")
    i = divisor ** 2
    while (i < MAX):
        array[i] = 0
        i += divisor
    if divisor > SQRT_MAX:
        with open("primes.txt", "a") as f:
            for i in range(0, SQRT_MAX):
                if array[i] == 1:
                    f.write(f"{i} ")
            f.write("\n")
        break

print("\033[92m\nSearch completed\033[0m")
for i in range(0, MAX):
    if array[i] == 1:
        divisor = i

print(f"\033[34mLargest prime number: {divisor}\033[0m")


def rev_num_sort(num):
    strs = str(num)
    return strs == "".join(sorted(strs, reverse=True))

primes = {}
for prime in range(0, MAX):
    if array[prime] == 1:
        primes[prime] = prime

REVERSABLE_PRIMES = 0
REVERSABLE_PRIMES_POWER_2_PRIMES = 0
for prime in primes:#in range(0, 10_000_000):
    if  (rev_num_sort(prime)):
        total = 1
        dictt = {}
        flag = True
        for i in str(prime):
            if i in dictt:
                flag = False
                break
            dictt[i] = 1
        if (flag == False):
            continue
        REVERSABLE_PRIMES += 1
        for i in str(prime):
            total += pow(2, int(i))
        if total in primes:
            REVERSABLE_PRIMES_POWER_2_PRIMES += 1
            print(prime)
        elif (total - 2) in primes:
            REVERSABLE_PRIMES_POWER_2_PRIMES += 1
            print(prime)
        else:
            total = 0
            for i in str(prime):
                total += pow(3, int(i))
            if total in primes:
                REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                print(prime)
            elif (total + 3) in primes:
                REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                print(prime)
            else:
                total = 1
                for i in str(prime):
                    total += pow(4, int(i))
                if total in primes:
                    REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                    print(prime)
                elif (total - 2) in primes:
                    REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                    print(prime)
                else:
                    total = 3
                    for i in str(prime):
                        total += pow(5, int(i))
                    if total in primes:
                        REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                        print(prime)
                    elif (total - 3) in primes:
                        REVERSABLE_PRIMES_POWER_2_PRIMES += 1
                        print(prime)
                    else:
                        print(f"\033[91m{prime}, {total - 2}, {total}\033[0m")

print(f"\033[34mREVERSABLE_PRIMES: {REVERSABLE_PRIMES}\033[0m")
print(f"\033[34mREVERSABLE_PRIMES_POWER_2_PRIMES: {REVERSABLE_PRIMES_POWER_2_PRIMES}\033[0m")
