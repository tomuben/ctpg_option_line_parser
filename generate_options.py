import argparse

def generate(lines, number_per_line):
    counter = 0
    for l in range(lines):
        option_line = ''
        for option_idx in range(number_per_line):
            option_line += f'%Option{counter} "some_value";'
            counter += 1
        print(option_line)

    print("Some code...")


def main():
    parser = argparse.ArgumentParser(
        prog='OptionGenerator',
        description='Generates a number of options')
    parser.add_argument('lines', type=int, metavar='N')
    parser.add_argument('numberperline', type=int, metavar='N')
    args = parser.parse_args()
    generate(args.lines, args.numberperline)


if __name__ == "__main__":
    main()