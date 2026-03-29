/**
 * Benchmark prompts implementation: 6 subjects × 3 levels × 10 questions.
 * Style: MMLU (general), GSM8K (math), logic puzzles, HumanEval/MBPP (code).
 */
#include "prompts.h"

namespace benchmark {

static std::vector<std::string> generalBasic() {
    return {
        "What is the capital of France? Answer in one word.",
        "How many continents are there on Earth? Answer with a number.",
        "What color is the sky on a clear day? One word.",
        "Who wrote Romeo and Juliet? Answer with the author's last name.",
        "What is 2 + 2? Answer with a number only.",
        "Which planet is known as the Red Planet? One word.",
        "What is the largest ocean on Earth? One word.",
        "How many days are in a leap year? Number only.",
        "What is the chemical symbol for water? (e.g. H2O)",
        "Who painted the Mona Lisa? Last name only.",
    };
}

static std::vector<std::string> generalMedium() {
    return {
        "Explain in 2-3 sentences: What causes the four seasons on Earth?",
        "What is the difference between a hypothesis and a theory in science? Answer in one short paragraph.",
        "Name three branches of the United States federal government and their main function.",
        "What is photosynthesis? Describe it in 2 sentences.",
        "What were the main causes of World War I? List up to three.",
        "Explain the water cycle in 2-3 sentences.",
        "What is the difference between DNA and RNA? One paragraph.",
        "Describe how a bill becomes a law in the US Congress in 3 steps.",
        "What is the greenhouse effect and why does it matter? Short answer.",
        "What are the three states of matter? Give one example of each.",
    };
}

static std::vector<std::string> generalAdvanced() {
    return {
        "Compare and contrast classical conditioning and operant conditioning. Give one example of each.",
        "Explain the significance of the Turing test in artificial intelligence and one major criticism of it.",
        "Discuss how the Treaty of Versailles contributed to the rise of fascism in Europe in the 1920s-30s.",
        "Describe the process of natural selection and how it can lead to speciation. Use 3-4 sentences.",
        "What is the prisoner's dilemma in game theory? Explain the payoff matrix and the Nash equilibrium.",
        "Summarize the main arguments for and against free trade in economics (2-3 sentences each).",
        "Explain the difference between nominal GDP and real GDP. Why do economists prefer real GDP for growth?",
        "What is the Heisenberg uncertainty principle? State it and give one consequence.",
        "Describe the structure and function of the mitochondria. Why is it called the powerhouse of the cell?",
        "What is the difference between correlation and causation? Give an example where they are confused.",
    };
}

static std::vector<std::string> mathBasic() {
    return {
        "If John has 5 apples and buys 3 more, how many apples does he have? Answer with a number.",
        "What is 12 × 4? Number only.",
        "A book has 80 pages. You read 25 pages. How many pages are left? Number only.",
        "What is 100 divided by 5? Number only.",
        "What is 7 + 8 + 5? Number only.",
        "A box has 6 rows of 4 cookies. How many cookies in total? Number only.",
        "What is 15 − 7? Number only.",
        "How many minutes are in 2 hours? Number only.",
        "What is 9 × 9? Number only.",
        "If a dozen eggs cost 3 dollars, how much does one egg cost in dollars? Number or fraction.",
    };
}

static std::vector<std::string> mathMedium() {
    return {
        "A store sells shirts for $15 each. They have a 20% off sale. What is the sale price of one shirt in dollars?",
        "A train travels 120 miles in 2 hours. What is its average speed in miles per hour?",
        "The sum of three consecutive integers is 72. What is the largest of the three?",
        "A rectangle has length 12 cm and width 5 cm. What is its area in square cm?",
        "Solve for x: 3x + 7 = 22. Give x only.",
        "A recipe for 4 people needs 2 cups of flour. How many cups are needed for 10 people?",
        "What is 15% of 80? Number only.",
        "A car uses 5 gallons of gas to travel 150 miles. How many miles per gallon does it get?",
        "What is the next number in the sequence: 2, 6, 12, 20, 30, ?",
        "A right triangle has legs 3 and 4. What is the length of the hypotenuse?",
    };
}

static std::vector<std::string> mathAdvanced() {
    return {
        "Find the derivative of f(x) = x^3 + 2x^2 - 5x + 1. Write the expression.",
        "Solve the quadratic equation x^2 - 5x + 6 = 0. Give both roots, comma-separated.",
        "A loan of $10,000 at 6% annual interest compounded yearly. What is the amount after 3 years? Round to 2 decimals.",
        "Integrate: integral of (2x + 1) dx. Give the antiderivative (use C for constant).",
        "In how many ways can 5 people be seated in a row of 5 chairs? Number only.",
        "The sum of an arithmetic sequence: first term 3, common difference 4, 10 terms. What is the sum?",
        "Log base 2 of 8 equals what? Number only.",
        "A circle has equation x^2 + y^2 = 25. What is its radius?",
        "What is the probability of rolling two fair dice and getting a sum of 7? Give as a fraction.",
        "Solve for x: 2^(x+1) = 16. Give x only.",
    };
}

static std::vector<std::string> logicalReasoningBasic() {
    return {
        "All cats are animals. Fluffy is a cat. What can we conclude about Fluffy? One sentence.",
        "If it rains, the ground is wet. The ground is wet. Can we conclude it rained? Answer yes or no and why in one sentence.",
        "Alice is taller than Bob. Bob is taller than Carol. Who is shortest? One name.",
        "No reptiles are mammals. A snake is a reptile. Is a snake a mammal? Yes or no.",
        "Every even number is divisible by 2. 14 is even. What can we say about 14? One short sentence.",
        "If A then B. A is true. What can we say about B? One word.",
        "Monday comes before Tuesday. Tuesday comes before Wednesday. What day comes after Monday? One word.",
        "All birds have wings. Tweety has wings. Can we conclude Tweety is a bird? Answer yes or no and one sentence.",
        "None of the boxes are red. This box is one of the boxes. Is this box red? Yes or no.",
        "Either P or Q. P is false. What must be true? One letter.",
    };
}

static std::vector<std::string> logicalReasoningMedium() {
    return {
        "Three friends: Alice, Bob, Carol. One always tells the truth, one always lies, one alternates. Alice says 'I am not the liar.' Bob says 'Alice is the liar.' Who is the truth-teller? Name only.",
        "Five people are in a line. A is not first. B is immediately behind A. C is last. D is not next to B. List the order from first to last (e.g. A,B,C,D,E).",
        "If the code for RED is 27 and the code for BLUE is 28, what number might GREEN be if the pattern is sum of letter positions? Give one number.",
        "A says B is lying. B says C is lying. C says A is lying. Only one tells the truth. Who tells the truth? One letter.",
        "In a race, Alice finished before Bob but after Carol. Dave finished after Bob. Who finished second? One name.",
        "All philosophers are thinkers. Some thinkers are writers. Can we conclude some philosophers are writers? Answer yes or no and one sentence.",
        "If no A are B, and some B are C, what can we conclude about A and C? One short sentence.",
        "Three boxes: one has two gold coins, one has two silver, one has one of each. You pick a gold coin from a random box. What is the probability the other coin in that box is gold? Fraction.",
        "What number comes next: 1, 1, 2, 3, 5, 8, 13, ?",
        "A drawer has 10 black and 10 white socks. How many socks must you take (without looking) to guarantee a matching pair? Number only.",
    };
}

static std::vector<std::string> logicalReasoningAdvanced() {
    return {
        "Formal logic: Convert 'If not P then Q' into an equivalent form using only AND, OR, NOT. Write the logical formula.",
        "Knights and knaves: On an island, knights always tell the truth and knaves always lie. A says 'B is a knight.' B says 'We are different types.' What is A? Knight or knave.",
        "There are 3 doors. Behind one is a car, behind the other two are goats. You pick door 1. The host opens door 3 (a goat). Should you switch to door 2 to maximize chance of the car? Yes or no, and one sentence.",
        "Given: (P implies Q) and (Q implies R). If not R, what can we deduce? One letter or expression.",
        "Five houses in a row, five colors, five nationalities, five drinks, five pets. The Englishman lives in the red house. The Spaniard has a dog. Coffee is drunk in the green house. The Ukrainian drinks tea. The green house is immediately to the right of the ivory house. Who drinks water? One nationality.",
        "In a group of 100 people, at least how many were born in the same month? Number only.",
        "Prove or disprove in one sentence: For all integers n, if n^2 is even then n is even.",
        "A bag has 3 red and 2 blue balls. You draw 2 without replacement. What is the probability both are red? Fraction.",
        "What is the next symbol in: O, T, T, F, F, S, S, ? (Hint: first letters of numbers.)",
        "If 'some A are B' and 'all B are C', can we conclude 'some A are C'? Yes or no and one sentence.",
    };
}

static std::vector<std::string> pythonBasic() {
    return {
        "Write a one-line Python expression that returns the sum of the list [1, 2, 3, 4, 5].",
        "What does len([10, 20, 30]) return in Python? Number only.",
        "Write a Python one-liner to get the first element of list L (variable L exists).",
        "What is the type of 3.14 in Python? One word.",
        "How do you write a comment in Python? Show one line example.",
        "What does 'hello'.upper() return in Python? Exact string in quotes.",
        "Write the Python boolean literal for false.",
        "What does range(3) produce in Python 3? Describe in one short phrase (e.g. 'numbers 0 to 2').",
        "How do you check if key 'x' exists in dictionary d? Write one line of code.",
        "What is the result of 10 // 3 in Python? Number only.",
    };
}

static std::vector<std::string> pythonMedium() {
    return {
        "Write a Python function that takes a list of numbers and returns the maximum. Signature: def max_of_list(nums):",
        "Write a list comprehension that produces the squares of integers from 0 to 9.",
        "Explain in one sentence: what does the 'with' statement do for file handling in Python?",
        "Write a Python snippet that opens file 'data.txt', reads one line, and closes the file (2-3 lines).",
        "What is the output of: print([x*2 for x in range(4)]) ? Exact output.",
        "Write a function that takes a string and returns True if it is a palindrome, else False. 3-5 lines.",
        "What is the difference between list and tuple in Python? One sentence each.",
        "How do you catch any exception in Python and store it in variable e? Write the except line.",
        "Write a Python one-liner to sort a list of strings by length.",
        "What does __init__ do in a Python class? One sentence.",
    };
}

static std::vector<std::string> pythonAdvanced() {
    return {
        "Implement a Python function that returns the n-th Fibonacci number using recursion. Signature: def fib(n):",
        "Write a generator in Python that yields even numbers starting from 0 (infinite). 3-4 lines.",
        "Explain the difference between *args and **kwargs in Python in 1-2 sentences.",
        "Write a decorator that prints 'before' and 'after' around a function call. 5-7 lines.",
        "What is the time complexity of looking up an element in a Python set? Big-O only.",
        "Write a context manager class that measures and prints the time taken inside the 'with' block. 6-8 lines.",
        "How does Python's GIL (Global Interpreter Lock) affect multithreading? One sentence.",
        "Write a function that flattens a nested list one level: [[1,2],[3,4]] -> [1,2,3,4]. 2-4 lines.",
        "What is the output of: (lambda x: x**2)(5) in Python? Number only.",
        "Implement a simple LRU cache in Python using OrderedDict (or describe in 2 sentences how you would).",
    };
}

static std::vector<std::string> javascriptBasic() {
    return {
        "What does typeof [] return in JavaScript? Exact string.",
        "Write a one-line JavaScript expression to get the length of array arr.",
        "What is the result of 1 + '2' in JavaScript? Exact value.",
        "How do you declare a constant in JavaScript? Show one line example.",
        "What does [1,2,3].push(4) return? (return value of push) Number or description.",
        "Write the JavaScript to get the first element of array a.",
        "What is the boolean value of null in JavaScript when used in an if? true or false.",
        "How do you write a template literal with a variable x in JavaScript? One line example.",
        "What does Array.isArray([]) return? true or false.",
        "Write one line to add key 'name' with value 'John' to object obj.",
    };
}

static std::vector<std::string> javascriptMedium() {
    return {
        "Write a JavaScript function that takes an array of numbers and returns the sum using reduce.",
        "What is the difference between == and === in JavaScript? One sentence.",
        "Write an arrow function that doubles a number: double = ...",
        "What does 'use strict' do in JavaScript? One sentence.",
        "Write a one-liner to create a new array that is the duplicate of arr with map (identity).",
        "Explain in one sentence: what is the event loop in JavaScript?",
        "Write a Promise that resolves to 42 after 1 second (setTimeout). 4-5 lines.",
        "What is the output of: console.log([] + []); in JavaScript? Exact output.",
        "How do you deep clone an object in JavaScript without libraries? One method name or one sentence.",
        "Write a function that returns true if argument is an array, false otherwise. One line.",
    };
}

static std::vector<std::string> javascriptAdvanced() {
    return {
        "Implement async/await: an async function fetchUserId that awaits a fetch to '/user' and returns the id from JSON.",
        "Explain in one sentence: what is closure in JavaScript and one use case.",
        "What is the difference between let, const, and var in terms of scope? One sentence each.",
        "Write a debounce function in JavaScript: debounce(fn, delay) returns a function that delays fn by delay ms.",
        "What is the prototype chain in JavaScript? One sentence.",
        "Write a recursive function that flattens a nested array one level in JavaScript.",
        "What does 'this' refer to in an arrow function versus a regular function? One sentence.",
        "Implement a simple Observable or event emitter: subscribe(cb) and emit(value). 5-7 lines.",
        "What is the output of: (function(){ return typeof arguments; })(); Exact string.",
        "How would you implement memoization for a function f in JavaScript? 2-3 sentences or 5 lines.",
    };
}

static std::vector<std::string> csharpBasic() {
    return {
        "What keyword do you use to define a class in C#? One word.",
        "How do you declare an integer variable in C#? One line example.",
        "What is the default value of a bool in C#? true or false.",
        "Write the C# line to create a list of strings: List<string> list = ...",
        "What does Console.WriteLine do? One short sentence.",
        "How do you make a class inherit from another in C#? Show the class declaration line.",
        "What is the entry point method name in a C# console app? Method name only.",
        "Write one line to get the length of string s in C#.",
        "What is the nullable type for int in C#? Write the type name.",
        "How do you compare two strings for equality in C# (value comparison)? One method or operator.",
    };
}

static std::vector<std::string> csharpMedium() {
    return {
        "Write a C# method that takes an array of int and returns the sum. Signature and body in 3-4 lines.",
        "What is the difference between struct and class in C#? One sentence each.",
        "Write a C# property with a private backing field (auto-style or full). 3 lines.",
        "What does the 'using' statement do for IDisposable in C#? One sentence.",
        "Write a C# foreach loop that prints each element of List<int> numbers.",
        "What is the difference between ref and out parameters in C#? One sentence.",
        "How do you declare an async method in C#? Show the method signature.",
        "Write a C# lambda expression that takes two ints and returns their sum.",
        "What is the purpose of the lock keyword in C#? One sentence.",
        "Write a C# switch expression (C# 8+) that maps int 1->'one', 2->'two', default->'other'. One expression.",
    };
}

static std::vector<std::string> csharpAdvanced() {
    return {
        "Implement a generic C# method that swaps two elements in a List<T> by index. Signature and body.",
        "Explain the difference between IEnumerable and IQueryable in C# in 1-2 sentences.",
        "Write a C# async method that awaits Task.Delay(1000) and then returns 42.",
        "What are extension methods in C#? How do you define one? One sentence and one line example.",
        "Describe in one sentence: what is the purpose of the volatile keyword in C#.",
        "Write a simple C# delegate type and assign a lambda to it. 2-3 lines.",
        "What is the difference between throw and throw ex in C# when rethrowing? One sentence.",
        "Implement a C# iterator (yield return) that yields 1, 2, 3. 4-5 lines.",
        "What is the role of the async state machine in C#? One sentence.",
        "Write a C# pattern matching switch (C# 8+) that matches on type (int, string, null). 5-6 lines.",
    };
}

std::vector<std::vector<std::vector<std::string>>> getAllPrompts() {
    std::vector<std::vector<std::vector<std::string>>> out;
    out.resize(6); // 6 subjects
    out[0] = { generalBasic(), generalMedium(), generalAdvanced() };
    out[1] = { mathBasic(), mathMedium(), mathAdvanced() };
    out[2] = { logicalReasoningBasic(), logicalReasoningMedium(), logicalReasoningAdvanced() };
    out[3] = { pythonBasic(), pythonMedium(), pythonAdvanced() };
    out[4] = { javascriptBasic(), javascriptMedium(), javascriptAdvanced() };
    out[5] = { csharpBasic(), csharpMedium(), csharpAdvanced() };
    return out;
}

} // namespace benchmark
