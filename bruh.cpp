#include <sys/mman.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <array>
#include <vector>

#define ASCII_ZERO 48

/*
uint32_t size of dictionary
...             ...
...             ...
...  dictionary ...
...             ...
...             ...
data
----
-----
-----
-----



*/

enum HephResult
{
    Success,
    Failure
};

struct MappedObjFile {
    char *base;
    /* this is void because I dont want to crawl down it,
    I just want a point that refers to the end of the memory region */
    void *end;
    size_t size_bytes;

    size_t vertices_size_bytes;
    size_t faces_size_bytes;
    size_t vertex_normals_size_bytes;

    size_t vertices_num;
    size_t faces_num;
    size_t vertex_normals_num;

    MappedObjFile() : base(nullptr), end(nullptr), size_bytes(0), 
        vertices_size_bytes(0), faces_size_bytes(0), vertex_normals_size_bytes(0),
        vertices_num(0), faces_num(0), vertex_normals_num(0) {}
};

struct Face
{
    uint32_t v1, v2, v3;
};

typedef struct Vertex
{
    float x, y, z;
} Vertex, VertexNormal;

void 
get_size_components(char *file, MappedObjFile& file_info)
{
    if (file_info.size_bytes <= 0)
        return;
    
    size_t faces = 0;
    size_t vertices = 0;
    size_t vertex_normals = 0;
    for (size_t i = 0; i < file_info.size_bytes; i++)
    {
        char val = *(file + i);
        switch (val)
        {
            case 'f':
                faces++;
                break;
            case 'v':
            {
                if (*(file + i + 1) == 'n')
                {
                    vertex_normals++;
                }
                vertices++;
            }
        }
    }
    file_info.vertices_num = vertices;
    file_info.faces_num = faces;
    file_info.vertex_normals_num = vertex_normals;

    file_info.vertices_size_bytes = vertices * sizeof(Vertex);
    file_info.faces_size_bytes = faces * sizeof(Face);
    file_info.vertex_normals_size_bytes = vertex_normals * sizeof(VertexNormal);
}

HephResult
map_obj_file(const std::string& path, MappedObjFile& mapped_file)
{
    int file_desc = open(path.c_str(), O_RDWR);

    struct stat status;
    if (fstat(file_desc, &status) < 0)
    {   
        std::cout << "failed to get status" << std::endl;
        return HephResult::Failure;
    }

    if (status.st_size <= 0)
    {   
        std::cout << "file empty" << std::endl;
        return  HephResult::Failure;
    }
    char *mapped_ptr = (char *)mmap(nullptr, status.st_size, PROT_READ, MAP_PRIVATE, file_desc, 0);
    if (mapped_ptr == MAP_FAILED)
    {
        std::cout << "map failed" << std::endl;
        return  HephResult::Failure;
    }
    close(file_desc);

    mapped_file.base = mapped_ptr;
    mapped_file.end = mapped_ptr + status.st_size;
    mapped_file.size_bytes = status.st_size;
    get_size_components(mapped_ptr, mapped_file);
    return  HephResult::Success;
}

void 
read_to_face(Face& face, char *start, char *end)
{   
    uint32_t multiplier = 1;
    uint32_t accum = 0;
    size_t number_num = 2;
    std::array<uint32_t, 3> real_numbers;
    while (--end != start)
    {   
        if (*end == ' ')
        {
            real_numbers[number_num] = accum;
            accum = 0;
            multiplier = 1;
            continue;
        }
        char val = *end - '0';
        accum += val * multiplier;
        multiplier *= 10;
    }
}   

void read_to_vertex(Vertex& vertex)
{

}

void 
seek_end(char **p_to_p, void *eof)
{
    while (**p_to_p != '\n' && *p_to_p <= eof)
        *p_to_p++;
}

int
main()
{
    std::string uncompressed_path = "/Users/macfarrell/vs code projects/compress_test/cheburashka.obj";
    MappedObjFile file{};
    HephResult result = map_obj_file(uncompressed_path, file);
    if (result == HephResult::Failure)
    {
        std::cout << "mapping failed";
        return 1;
    }

    std::vector<Face> faces;
    faces.reserve(file.faces_num);
    std::vector<Vertex> vertices;
    vertices.reserve(file.vertices_num);

    for (size_t i = 0; i <= file.size_bytes; i++)
    {
        char *ptr = file.base + i;
        switch (*ptr)
        {
            case 'f':
            {
                char *j = ptr + 2;
                seek_end(&j, file.end);

                Face face{};
                read_to_face(face, ptr, j);
                faces.push_back(face);
    
                ptr = j + 1; 
            }
            case 'v':
            {
                
            }
        }
    }
    
    munmap((void *)file.base, file.size_bytes);
}
