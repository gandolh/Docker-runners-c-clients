def is_prime(n):
    if n <= 1:
        return False
    for i in range(2, n):
        if n % i == 0:
            return False
    return True

count = 0
num = 2
prime_numbers = []

while count < 4:
    if is_prime(num):
        prime_numbers.append(num)
        count += 1
    num += 1

print("The first 4 prime numbers are:", prime_numbers)