#include <fstream>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <string>
#include <type_traits>

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG >= 201703L, "This program requires at least C++17");
#else
static_assert(__cplusplus >= 201703L, "This program requires at least C++17");
#endif

using std::string, std::vector;

struct Vertex
{
	float value1, value2, value3;
};

struct Normal
{
	float value1, value2, value3;
};

struct Face
{
	int value1, value2, value3;
};

bool ReadObj(const string& filename, vector<Face>& faces, vector<Vertex>& vertices, vector<Normal>& normals, vector<string>& comments)
{
	if (std::ifstream input{ filename }; input.is_open())
	{
		bool isOk{ true };

		string line{};
		int lineCounter{};

		//Read variables
		char flagCharacter{};
		Face face{};
		Vertex vertex{};
		Normal normal{};

		while (input.good())
		{
			//Flag
			if (!(input >> flagCharacter))
			{
				continue;
			}

			//Read values
			switch (flagCharacter)
			{
				//Faces
			case 'f':
				if (input >> face.value1 && input >> face.value2 && input >> face.value3)
				{
					faces.push_back(face);
				}
				else
				{
					std::cout << "Invalid face value: line " << lineCounter << '\n';
					isOk = false;
				}
				break;
				//Vertices or normals
			case 'v':
				input >> flagCharacter;
				if (flagCharacter == 'n')
				{
					//Normals
					if (input >> normal.value1 && input >> normal.value2 && input >> normal.value3)
					{
						normals.push_back(normal);
					}
					else
					{
						std::cout << "Invalid normal value: line " << lineCounter << '\n';
						isOk = false;
					}
				}
				else
				{
					//Vertices
					input.unget();

					if (input >> vertex.value1 && input >> vertex.value2 && input >> vertex.value3)
					{
						vertices.push_back(vertex);
					}
					else
					{
						std::cout << "Invalid vertex value: line " << lineCounter << '\n';
						isOk = false;
					}
				}
				break;
			}

			//Go to the next line
			std::getline(input, line);
			++lineCounter;

			//Comment is on this line
			if (flagCharacter == '#')
			{
				comments.push_back(line);
			}
		}

		input.close();
		return isOk;
	}

	return false;
}

bool ConvertTobObj(const std::string& filename)
{
	vector<Face> faces{};
	vector<Vertex> vertices{};
	vector<Normal> normals{};
	vector<string> comments{};

	if (ReadObj(filename, faces, vertices, normals, comments))
	{
		//Create .bObj verion
		const string outputName{ filename.substr(0,filename.find('.') + 1) + "bObj" };

		if (std::ofstream output{ outputName, std::ios::binary }; output.is_open())
		{
			size_t size{};
			
			//Comments
			size = comments.size();
			output.write((const char*)&size, sizeof(size));
			
			for (const auto& comment : comments)
			{
				size = comment.size();
				output.write((const char*)&size, sizeof(size));

				output.write(comment.data(), size);
			}

			//Vertices
			size = vertices.size();
			output.write((const char*)&size, sizeof(size));
			
			for (const auto& vertex : vertices)
			{
				output.write((const char*)&vertex, sizeof(vertex));
			}
			
			//Faces
			size = faces.size();
			output.write((const char*)&size, sizeof(size));
			
			for (const auto& face : faces)
			{
				output.write((const char*)&face, sizeof(face));
			}

			//Normals
			size = normals.size();
			output.write((const char*)&size, sizeof(size));

			for (const auto& normal : normals)
			{
				output.write((const char*)&normal, sizeof(normal));
			}

			output.close();
		}

		return true;
	}
	else
	{
		std::cout << "Reading .Obj failed\n";
		return false;
	}
}

bool ReadbObj(const string& filename, vector<Face>& faces, vector<Vertex>& vertices, vector<Normal>& normals, vector<string>& comments)
{
	if (std::ifstream input{ filename, std::ios::binary }; input.is_open()) 
	{
		size_t size{};

		Vertex vertex{};
		Normal normal{};
		Face face{};
		string comment{};

		//Comments
		if (input.read((char*)&size, sizeof(size)))
		{
			size_t length{};

			//Line
			for (size_t index{}; index < size; ++index)
			{
				if (input.read((char*)&length, sizeof(length)))
				{
					comment.resize(length,'\0');

					input.read(reinterpret_cast<char*>(comment.data()), length);
					comments.push_back(comment);
				}
				else
				{
					std::cout << "Reading number of characters (comment) failed\n";

					input.close();
					return false;
				}
			}
		}
		else
		{
			std::cout << "Reading number of comments failed\n";

			input.close();
			return false;
		}

		//Vertices
		if (input.read((char*)&size, sizeof(size)))
		{
			for (size_t index{}; index < size; ++index)
			{
				input.read((char*)&vertex, sizeof(vertex));
				vertices.push_back(vertex);
			}
		}
		else
		{
			std::cout << "Reading number of vertices failed\n";

			input.close();
			return false;
		}
		
		//Faces
		if (input.read((char*)&size, sizeof(size)))
		{		
			for (size_t index{}; index < size; ++index)
			{
				input.read((char*)&face, sizeof(face));
				faces.push_back(face);
			}
		}
		else
		{
			std::cout << "Reading number of faces failed\n";

			input.close();
			return false;
		}

		//Normals
		if (input.read((char*)&size, sizeof(size)))
		{
			for (size_t index{}; index < size; ++index)
			{
				input.read((char*)&normal, sizeof(normal));
				normals.push_back(normal);
			}
		}
		else
		{
			std::cout << "Reading number of normals failed\n";

			input.close();
			return false;
		}

		input.close();
		return true;
	}

	return false;
}

bool ConvertToObj(const string& filename, const string& outputName)
{
	vector<Vertex> vertices{};
	vector<Normal> normals{};
	vector<Face> faces{};
	vector<string> comments{};

	if (ReadbObj(filename, faces, vertices, normals, comments))
	{
		std::stringstream ss{};
		constexpr int precision{ 7 };

		//Print comments
		for (const auto& comment : comments)
		{
			ss << "#" << comment << '\n';
		}

		//Print vertices
		for (const auto& vertex : vertices)
		{
			ss << "v " << std::scientific << std::setprecision(precision) << vertex.value1 << ' ' << vertex.value2 << ' ' << vertex.value3 << '\n';
		}

		//Print faces
		for (const auto& face : faces)
		{
			ss << "f " << face.value1 << ' ' << face.value2 << ' ' << face.value3 << '\n';
		}

		//Print normals
		for (const auto& normal : normals)
		{
			ss << "vn " << std::scientific << std::setprecision(precision) << normal.value1 << ' ' << normal.value2 << ' ' << normal.value3 << '\n';
		}

		//Prints to file
		if (std::ofstream output{ outputName, std::ios::binary }; output.is_open())
		{
			output << ss.str();
			output.close();
		}
		else
		{
			return false;
		}

		return true;
	}
	else
	{
		std::cout << "Reading .bObj failed\n";
		return false;
	}
}

int main()
{
	if (ConvertTobObj("low poly stanford bunny.obj"))
	{
		std::cout << "Conversion to bObj went fine!\n";
	}
	
	if (ConvertToObj("low poly stanford bunny.bObj", "test.obj"))
	{
		std::cout << "Conversion to Obj went fine!\n";
	}
}
