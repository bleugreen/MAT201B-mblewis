// Assignment 1 for MAT201B
// Mitchell Lewis / mblewis@ucsb.edu - 1/27/19

// reverses the words of a user-inputted sentence
// i.e. "this is the truth" becomes "siht si eht hturt"

#include <iostream>
#include <string>
#include <sstream>
using namespace std;
int main() {
    while (true) {
        printf("Type a sentence (then hit return): ");
        string line, reverseLine;
        getline(cin, line);
        
        // added a "quit on blank line" condition to exit program easier
        if (!cin.good() || line.length() < 1) {
            printf("Done\n");
            return 0;
        }
        
        // istringstream sentence splitting from GeeksforGeeks - "Split a sentence into words in C++"
        // Used to split string around spaces.
        istringstream ss(line);
        
        // Traverse through all words
        do {
            // Read a word
            string word;
            ss >> word;
            
            // build reverseWord by iterating backwards through word
            int i = word.length() - 1;
            string reverseWord = "";
            while(i >= 0){
                reverseWord += word[i];
                i--;
            }
            
            // add reverseWord to reverseLine
            reverseLine += ' ' + reverseWord;
            
            // While there is more to read
        } while (ss);
        
        
        // print final reversed sentence
        cout << reverseLine << endl;
        
    }
}


