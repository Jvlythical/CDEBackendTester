#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {

	string s;

	cout << "Please enter a string: " << endl;
	cin >> s;
	cout << s << endl;

	cout << argc << endl;
	cout << argv[1] << endl;
	
	return 0;
}
