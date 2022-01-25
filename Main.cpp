#include <iostream>
#include <bitset>
#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <functional>
#include <cstdint>

#include <cassert>

const unsigned int kBase = 255;

std::vector<unsigned int> generatePrimes(const unsigned int & highestNumber) {
	std::vector<unsigned int> primes;
	for (unsigned int i=2; i < highestNumber; i++) {
		bool isPrime = true;
		for(const unsigned int & prime : primes) {
			if((i % prime) == 0) {
				isPrime = false;
			}
		}
		if (isPrime) {
			primes.push_back(i);
		}
	}
	return primes;
}


const std::vector<unsigned int> primes = generatePrimes(kBase);




typedef struct {
	std::string inputFilename;
	std::string outputFilename;
	bool encode = true;
	bool ascii = false;
	bool verbose = false;

	void setInputFile(const std::string & inputFilename) {
		this->inputFilename = inputFilename;
	}

	void setOutputFile(const std::string & outputFilename) {
		this->outputFilename = outputFilename;
	}

	void setEncode(const bool & encode) {
		this->encode = encode;
	}

	void setAscii() {
		this->ascii = true;
	}

	void enableVerbose() {
		verbose = true;
	}

	bool isValid() {
		return !inputFilename.empty() && !outputFilename.empty();
	}
} RunSettings_s;

RunSettings_s parseArguments(std::vector<std::string> unprocessedArguments) {
	//parse unprocessedArguments
	RunSettings_s runSettings = {};

	if(unprocessedArguments.empty()) {
		return runSettings;
	}

	//set pair argument handlers
	const std::unordered_map<std::string, std::function<void(std::string)> > pairArgumentHandlers = {
		{"-i", std::bind(&RunSettings_s::setInputFile, &runSettings, std::placeholders::_1)},
		{"-o", std::bind(&RunSettings_s::setOutputFile, &runSettings, std::placeholders::_1)}
	};
	//set single argument handlers
	const std::unordered_map<std::string, std::function<void()> > singleArgumentHandlers = {
		{"-v", std::bind(&RunSettings_s::enableVerbose, &runSettings)},
		{"-e", std::bind(&RunSettings_s::setEncode, &runSettings, true)},
		{"-d", std::bind(&RunSettings_s::setEncode, &runSettings, false)},
		{"-a", std::bind(&RunSettings_s::setAscii, &runSettings)},
	};

	//parse pair arguments
	std::vector<std::pair<std::string, std::string> > pairedArguments;
	for (unsigned int i=0; i < unprocessedArguments.size()-1; i++) {
		const std::string & argument = unprocessedArguments[i];
		if (pairArgumentHandlers.count(argument) != 0) {
			//store argument pair
			pairedArguments.push_back(std::make_pair(argument, unprocessedArguments[i+1]));
			//erase processed arguments
			unprocessedArguments.erase(unprocessedArguments.begin() + i+1);
			unprocessedArguments.erase(unprocessedArguments.begin() + i);
			i--;
			continue;
		}
	}

	//parse single arguments
	std::vector<std::string> singleArguments;
	for (unsigned int i=0; i < unprocessedArguments.size(); i++) {
		const std::string & argument = unprocessedArguments[i];
		if (singleArgumentHandlers.count(argument) != 0) {
			//store argument
			singleArguments.push_back(argument);
			//erase processed argument
			unprocessedArguments.erase(unprocessedArguments.begin() + i);
			i--;
			continue;
		}
	}

	//handle pair arguments
	for (auto pairedArgument : pairedArguments){
		const std::string & key = pairedArgument.first;
		const std::string & value = pairedArgument.second;
		//find and call handler
		pairArgumentHandlers.at(key)(value);
	}

	//handle single
	for (auto singleArgument : singleArguments){
		//find and call handler
		singleArgumentHandlers.at(singleArgument)();
	}

	return runSettings;
}




void getPrimeCounts(unsigned int number, std::vector<unsigned int> & primeCounts) {
	primeCounts.assign(primes.size(), 0);
	primeCounts.push_back(0); 	//one more for '1' (which is the first in primeCounts, not the last)
	if (number == 1) {
		primeCounts[0] = 1;
		return;
	} else if(number == 0) {
		return;
	}

	for (unsigned int i=0; i < primes.size(); i++) {
		auto prime = primes[i];
		while ((number % prime) == 0) {
			number /= prime;
			primeCounts[i+1]++;
		}
	}
	assert(number == 1 || number == 0);
}

void convertPrimeCountToEncodingFragments(std::vector<unsigned int> primeCounts, std::vector<uint64_t> & encodingFragments) {
	assert(kBase <= 255); 	//NOTE: 'encodingFragments' uses uint64_t because it can house all the primes within 255.

	bool encodingComplete;
	do {
		encodingComplete = true;
		uint64_t encodingFragment = 0;
		for (unsigned int i=0; i < primeCounts.size(); i++) {
			unsigned int & primeCount = primeCounts[i];
			if (primeCount > 0) {
				encodingFragment |= 1 << i;
				primeCount--;
				if (primeCount > 0) {
					//if it's still above 0, then there's more to encode (but that'll go in the next encodingFragment(s))
					encodingComplete = false;
				}
			}
		}
		encodingFragments.push_back(encodingFragment);
	} while (!encodingComplete);
}

bool encodeAscii(const RunSettings_s & runSettings) {
	bool ret = true;
	std::ifstream inputFileStream(runSettings.inputFilename, std::ios::binary);
	std::ofstream outputFileStream(runSettings.outputFilename);

	char byte = 0;

	while (inputFileStream.get(byte)) {
		//get prime counts
		std::vector<unsigned int> primeCounts;
		getPrimeCounts(byte, primeCounts);

		bool theFirst = true;
		for (unsigned int primeIndex = 0; primeIndex < primeCounts.size(); primeIndex++) {
			if (primeCounts[primeIndex] > 0) {
				if(!theFirst) {
					outputFileStream << ';';
				}
				theFirst = false;
				outputFileStream << primeIndex << ":" << primeCounts[primeIndex];
			}
		}
		outputFileStream.put('\n'); 	//a line per byte
	}

	// if not end-of-file (eof), then error occured during reading
	if (!inputFileStream.eof()) {
		ret = false;
	}

	return ret;
}

bool encode(const RunSettings_s & runSettings) {
	bool ret = true;
	std::ifstream inputFileStream(runSettings.inputFilename, std::ios::binary);
	std::ofstream outputFileStream(runSettings.outputFilename, std::ios::binary);

	char byte = 0;
	while (inputFileStream.get(byte)) {
		//get prime counts
		std::vector<unsigned int> primeCounts;
		getPrimeCounts(byte, primeCounts);

		//conver prime counts to encoding fragments
		std::vector<uint64_t> encodingFragments;
		convertPrimeCountToEncodingFragments(primeCounts, encodingFragments);

		//together these encoding fragments make the data for the encoded byte
		//Write it to the output file
		//write how many fragments
		uint8_t numberOfFragments = encodingFragments.size();
		outputFileStream.write(reinterpret_cast<const char*>(&numberOfFragments), sizeof(numberOfFragments));
		//write the fragments
		for (const uint64_t & encodingFragment : encodingFragments) {
			outputFileStream.write(reinterpret_cast<const char*>(&encodingFragment), sizeof(encodingFragment));
		}
		// std::cout << "encode:[" << std::endl;
		// for (const uint64_t & encodingFragment : encodingFragments) {
		// 	std::cout << "\t" << std::bitset<64>(encodingFragment) << std::endl;
		// }
		// std::cout << "]" << std::endl;
	}

	// if not end-of-file (eof), then error occured during reading
	if (!inputFileStream.eof()) {
		ret = false;
	}

	return ret;
}

uint8_t convertFragmentsIntoByte(const std::vector<uint64_t> & fragments) {
	uint8_t ret = 1;
	bool anyBitSet = false;
	for (uint64_t fragment : fragments) {
		for (unsigned int i=1; i < sizeof(fragment) * 8; i++) {
			if (fragment & (1ull<<i)) {
				ret *= primes[i-1];
				anyBitSet = true;
			}
		}
	}
	if (anyBitSet) {
		return ret;
	} else {
		return 0;
	}
}

bool decodeAscii(const RunSettings_s & runSettings) {
	bool ret = true;
	std::ifstream inputFileStream(runSettings.inputFilename);
	std::ofstream outputFileStream(runSettings.outputFilename, std::ios::binary);

	char byte = 0;

	std::string line;

	while (std::getline(inputFileStream, line)) {
		uint8_t decodedByte = 1;
		if (line.empty()) {
			decodedByte = 0;
		} else {
			std::stringstream ssLine(line);
			char semicolon;
			do {
				unsigned int primeIndex=0;
				char colon;
				unsigned int primeCount=0;
				ssLine >> primeIndex;
				ssLine.get(colon);
				ssLine >> primeCount;
				if (primeIndex > 0) {
					decodedByte *= std::pow(primes[primeIndex-1], primeCount);
				}
			} while(ssLine.get(semicolon));
		}

		outputFileStream.put(decodedByte);
	}

	// if not end-of-file (eof), then error occured during reading
	if (!inputFileStream.eof()) {
		ret = false;
	}

	return ret;
}

bool decode(const RunSettings_s & runSettings) {
	bool ret = true;
	std::ifstream inputFileStream(runSettings.inputFilename, std::ios::binary);
	std::ofstream outputFileStream(runSettings.outputFilename, std::ios::binary);

	char byte = 0;
	while (inputFileStream.get(byte)) {
		//first byte indicates number of fragments
		const uint8_t numberOfFragments = byte;
		std::vector<uint64_t> fragments;
		for (unsigned int i=0; i < numberOfFragments; i++) {
			uint64_t fragment = 0;
			inputFileStream.read(reinterpret_cast<char*>(&fragment), sizeof(fragment));

			if(!inputFileStream) {
				return false;
			}
			fragments.push_back(fragment);
		}

		uint8_t decodedByte = convertFragmentsIntoByte(fragments);
		outputFileStream.put(decodedByte);
	}

	// if not end-of-file (eof), then error occured during reading
	if (!inputFileStream.eof()) {
		ret = false;
	}

	return ret;
}


int main(int argc, char ** argv) {
	//process arguments
	std::vector<std::string> unprocessedArguments;
	for (int i=1; i < argc; i++) {
		unprocessedArguments.push_back(std::string(argv[i]));
	}

	RunSettings_s runSettings = parseArguments(unprocessedArguments);
	
	if (!runSettings.isValid()) {
		return -1;
	}

	if (!runSettings.ascii) {
		if (runSettings.encode) {
			if (!encode(runSettings)) {
				return -1;
			}
		} else {
			if (!decode(runSettings)) {
				return -1;
			}
		}
	} else {
		if (runSettings.encode) {
			if (!encodeAscii(runSettings)) {
				return -1;
			}
		} else {
			if (!decodeAscii(runSettings)) {
				return -1;
			}
		}
	}

	return 0;
}