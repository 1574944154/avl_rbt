import random


with open("nums.txt", "w") as f:
    nums = [str(i) for i in range(10000000)]
    random.shuffle(nums)
    f.write(" ".join(nums))