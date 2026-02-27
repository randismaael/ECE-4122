// ============================================
// Author: Rand Ismaael
// Class: ECE 4122-A
// Last Date Modified: 09/08/2024

// Description: 
// Finds longest sum of consecutive primes equal to or less than a given input number.
// ============================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ============================================
// Function: isPrime
// Purpose: Checks if a number is prime.
// Input: Unsigned long number to check.
// Output: True if prime, false otherwise.
// ============================================
bool isPrime(unsigned long num) 
{
    if (num <= 1) return false;
    for (unsigned long i = 2; i * i <= num; i++) 
    {
        if (num % i == 0) return false;
    }
    return true;
}

// ============================================
// Function: isValid
// Purpose: Checks if an input is a valid number
// Input: User input
// Output: True if valid, false otherwise
// ============================================

bool isValid(const string& input)
{
    if (input.empty()) 
    {
        return false;
    }
    for (char c : input) 
    {
        if (!isdigit(c))
        { 
        return false;
        }
    }
    return true;
}

// ============================================
// Function: main
// Purpose: Main program loop to get user input and find longest sum of consecutive primes.
// Input: None
// Output: Longest sum of consecutive primes and the primes that make up the sum.
// ============================================
int main()
{
    while (true) 
    {
        cout << "Enter a natural number: ";
        string userInput;
        cin >> userInput;
        
        // checks if input is valid
        if (!isValid(userInput)) 
        {
            cout << "Invalid input. Please enter a natural number." << endl;
            continue;
        }

        unsigned long N = stoul(userInput);

        // exit condition
        if (N == 0) 
        {
            cout << "Program Terminated" << endl;
            cout << "Have a nice day!" << endl;
            break;
        }

        // checks if input is larger than 2^32
        if (N >= ((unsigned long) 1<< 32))
        {
            cout << "Input is too large." << endl;
            continue;
        }

        unsigned long largestSum = 0;
        int maxCount = 0;
        vector<unsigned long> primesList;
        vector<unsigned long> allPrimes;
        for (unsigned long i = 2; i <= N; i++) 
        {
            if (isPrime(i)) 
            {
                allPrimes.push_back(i);
            }
        }

        for (size_t start = 0; start < allPrimes.size(); start++) 
        {
            unsigned long sum = 0;
            int count = 0;
            vector<unsigned long> primes;

            // finds consecutive primes starting from index 'start'
            for (size_t num = start; num < allPrimes.size(); num++) 
            {
                sum += allPrimes[num];
                primes.push_back(allPrimes[num]);
                count++;

                if (sum > N) break;

                // updates largest sum if current is longer and sum itself is prime
                if (isPrime(sum) && count > maxCount) 
                {
                    maxCount = count;
                    largestSum = sum;
                    primesList = primes;
                }
            }
        }

        cout << "The answer is " << largestSum << " with " << maxCount << " terms: ";
        for (int i = 0; i < primesList.size(); i++) 
        {
            if (i > 0) 
            {
                cout << " + ";
            }
            cout << primesList[i];
        }
        cout << endl;
    }
    return 0;
}