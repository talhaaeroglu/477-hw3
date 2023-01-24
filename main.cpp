#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <queue>
#include <cstring>
#include <string>
#include <stack>
#include <stack>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>    // The GL Header File
#include <GL/gl.h>      // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include <chrono>
#include <unistd.h> // for linux 
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint gProgram[4];
GLint gIntensityLoc;

float gIntensity = 100;
int grid_width, grid_height;
int gWidth = 640, gHeight = 600;
int moves = 0, score = 0;
bool lockPop = false;

glm::vec3 colorArray[5] = {
    glm::vec3(1, 0.2, 0.6),
    glm::vec3(0.1, 0.8, 0.1),
    glm::vec3(0.1, 0.8, 0.8),
    glm::vec3(0.1, 0.1, 0.8),
    glm::vec3(0.9, 0.1, 0.1),
};

struct Fistik
{
    glm::vec3 color;
    int colorId;
    bool isClicked = false;
    bool show = true;
    bool isMoving = false;
    float yPos = 0;
    float scaleFactor = 0;
    int yOffset = 0;

    bool operator==(Fistik &obj)
    {
        return (color.x == obj.color.x && color.y == obj.color.y && color.z == obj.color.z);
    }
    int original_j;
};

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) {}
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
    GLfloat x, y, z;
};

struct Face
{
    Face(int v[], int t[], int n[])
    {
        vIndex[0] = v[0];
        vIndex[1] = v[1];
        vIndex[2] = v[2];
        tIndex[0] = t[0];
        tIndex[1] = t[1];
        tIndex[2] = t[2];
        nIndex[0] = n[0];
        nIndex[1] = n[1];
        nIndex[2] = n[2];
    }
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

std::vector<std::vector<Fistik>> colorGrid;

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    GLuint Advance;     // Horizontal offset to advance to next glyph
};

struct ComparePair
{
    bool operator()(const std::pair<int, int> &p1, const std::pair<int, int> &p2)
    {
        return p1.second > p2.second;
    }
};

void match_and_pop(int i, int j, bool click = false)
{

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, ComparePair> objectsToPop;
    objectsToPop.push({i, j});
    bool matched = false;

    // Check Vertical
    int count = 1;
    Fistik popObj = colorGrid[i][j];
    if (!lockPop)
    {
        for (int col = j - 1; col >= 0; col--)
        {
            Fistik currObj = colorGrid[i][col];
            if (currObj == popObj)
            {
                count++;
                objectsToPop.push({i, col});
            }
            else
                break;
        }

        for (int col = j + 1; col < grid_height; col++)
        {
            Fistik currObj = colorGrid[i][col];
            if (currObj == popObj)
            {
                count++;
                objectsToPop.push({i, col});
            }
            else
                break;
        }

        if (count >= 3)
        {
            matched = true;
            int offset = 0;
            while (!objectsToPop.empty())
            {
                auto it = objectsToPop.top();
                int x = it.first, y = it.second;
                objectsToPop.pop();
                colorGrid[x][y].isClicked = true;
                colorGrid[x][y].yOffset = offset;
                ++offset;
                ++score;
            }
        }
        // IF COUNT < 2, EMPTY STACK
        while (!objectsToPop.empty())
        {
            objectsToPop.pop();
        }

        // Check Horizontal
        count = 1;
        if (!matched)
            objectsToPop.push({i, j});
        for (int row = i - 1; row >= 0; row--)
        {
            Fistik currObj = colorGrid[row][j];
            if (currObj == popObj)
            {
                count++;
                objectsToPop.push({row, j});
            }
            else
                break;
        }

        for (int row = i + 1; row < grid_width; row++)
        {
            Fistik currObj = colorGrid[row][j];
            if (currObj == popObj)
            {
                count++;
                objectsToPop.push({row, j});
            }
            else
                break;
        }

        if (count >= 3)
        {
            matched = true;
            while (!objectsToPop.empty())
            {
                auto it = objectsToPop.top();
                int x = it.first, y = it.second;
                objectsToPop.pop();
                colorGrid[x][y].isClicked = true;
                ++score;
            }
        }
        if (click)
        {
            ++moves;
            if (!matched)
            {
                colorGrid[i][j].isClicked = true;
                ++score;
            }
        }
    }
}

std::map<GLchar, Character> Characters;

bool ParseObj(const string &fileName)
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
                    char c;
                    int vIndex[3], nIndex[3], tIndex[3];
                    str >> vIndex[0];
                    str >> c >> c; // consume "//"
                    str >> nIndex[0];
                    str >> vIndex[1];
                    str >> c >> c; // consume "//"
                    str >> nIndex[1];
                    str >> vIndex[2];
                    str >> c >> c; // consume "//"
                    str >> nIndex[2];

                    assert(vIndex[0] == nIndex[0] &&
                           vIndex[1] == nIndex[1] &&
                           vIndex[2] == nIndex[2]); // a limitation for now

                    // make indices start from 0
                    for (int c = 0; c < 3; ++c)
                    {
                        vIndex[c] -= 1;
                        nIndex[c] -= 1;
                        tIndex[c] -= 1;
                    }

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            // data += curLine;
            if (!myfile.eof())
            {
                // data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    /*
    for (int i = 0; i < gVertices.size(); ++i)
    {
        Vector3 n;

        for (int j = 0; j < gFaces.size(); ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                if (gFaces[j].vIndex[k] == i)
                {
                    // face j contains vertex i
                    Vector3 a(gVertices[gFaces[j].vIndex[0]].x,
                              gVertices[gFaces[j].vIndex[0]].y,
                              gVertices[gFaces[j].vIndex[0]].z);

                    Vector3 b(gVertices[gFaces[j].vIndex[1]].x,
                              gVertices[gFaces[j].vIndex[1]].y,
                              gVertices[gFaces[j].vIndex[1]].z);

                    Vector3 c(gVertices[gFaces[j].vIndex[2]].x,
                              gVertices[gFaces[j].vIndex[2]].y,
                              gVertices[gFaces[j].vIndex[2]].z);

                    Vector3 ab = b - a;
                    Vector3 ac = c - a;
                    Vector3 normalFromThisFace = (ab.cross(ac)).getNormalized();
                    n += normalFromThisFace;
                }

            }
        }

        n.normalize();

        gNormals.push_back(Normal(n.x, n.y, n.z));
    }
    */

    assert(gVertices.size() == gNormals.size());

    return true;
}

bool ReadDataFromFile(
    const string &fileName, ///< [in]  Name of the shader file
    string &data)           ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint &program, const string &filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar *shader = (const GLchar *)shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint &program, const string &filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar *shader = (const GLchar *)shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();
    gProgram[2] = glCreateProgram();
    gProgram[3] = glCreateProgram();

    createVS(gProgram[0], "vert0.glsl");
    createFS(gProgram[0], "frag0.glsl");

    createVS(gProgram[1], "vert1.glsl");
    createFS(gProgram[1], "frag1.glsl");

    createVS(gProgram[3], "vert0.glsl");
    createFS(gProgram[3], "frag0.glsl");

    createVS(gProgram[2], "vert_text.glsl");
    createFS(gProgram[2], "frag_text.glsl");

    glBindAttribLocation(gProgram[0], 0, "inVertex");
    glBindAttribLocation(gProgram[0], 1, "inNormal");
    glBindAttribLocation(gProgram[1], 0, "inVertex");
    glBindAttribLocation(gProgram[1], 1, "inNormal");
    glBindAttribLocation(gProgram[2], 2, "vertex");
    glBindAttribLocation(gProgram[3], 0, "inVertex");
    glBindAttribLocation(gProgram[3], 1, "inNormal");

    glLinkProgram(gProgram[0]);
    glLinkProgram(gProgram[1]);
    glLinkProgram(gProgram[3]);
    glLinkProgram(gProgram[2]);
    glUseProgram(gProgram[0]);

    gIntensityLoc = glGetUniformLocation(gProgram[0], "intensity");
    cout << "gIntensityLoc = " << gIntensityLoc << endl;
    glUniform1f(gIntensityLoc, gIntensity);
}

void initVBO()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &gVertexAttribBuffer);
    glGenBuffers(1, &gIndexBuffer);

    assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    gVertexDataSizeInBytes = gVertices.size() * 3 * sizeof(GLfloat);
    gNormalDataSizeInBytes = gNormals.size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);
    GLfloat *vertexData = new GLfloat[gVertices.size() * 3];
    GLfloat *normalData = new GLfloat[gNormals.size() * 3];
    GLuint *indexData = new GLuint[gFaces.size() * 3];

    float minX = 1e6, maxX = -1e6;
    float minY = 1e6, maxY = -1e6;
    float minZ = 1e6, maxZ = -1e6;

    for (int i = 0; i < gVertices.size(); ++i)
    {
        vertexData[3 * i] = gVertices[i].x;
        vertexData[3 * i + 1] = gVertices[i].y;
        vertexData[3 * i + 2] = gVertices[i].z;
    }

    for (int i = 0; i < gNormals.size(); ++i)
    {
        normalData[3 * i] = gNormals[i].x;
        normalData[3 * i + 1] = gNormals[i].y;
        normalData[3 * i + 2] = gNormals[i].z;
    }

    for (int i = 0; i < gFaces.size(); ++i)
    {
        indexData[3 * i] = gFaces[i].vIndex[0];
        indexData[3 * i + 1] = gFaces[i].vIndex[1];
        indexData[3 * i + 2] = gFaces[i].vIndex[2];
    }

    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    // glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x};
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init(string objFileName)
{
    // ParseObj("armadillo.obj");
    ParseObj(objFileName);
    glEnable(GL_DEPTH_TEST);
    initShaders();
    initFonts(gWidth, gHeight);
    initVBO();
}

void drawModel()
{
    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

    glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void renderText(const std::string &text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            {xpos, ypos + h, 0.0, 0.0},
            {xpos, ypos, 0.0, 1.0},
            {xpos + w, ypos, 1.0, 1.0},

            {xpos, ypos + h, 0.0, 0.0},
            {xpos + w, ypos, 1.0, 1.0},
            {xpos + w, ypos + h, 1.0, 0.0}};

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ypos = height - ypos;

        // Normalize the mouse position
        float norm_x = (xpos / width) * 20.0f - 10.0f;
        float norm_y = (ypos / height) * 20.0f - 10.0f;
        // std::cout << "norm_x : " << norm_x << std::endl;
        // std::cout << "norm_y : " << norm_y << std::endl;

        int grid_x = (norm_x + 9.0f) / (18.0f / grid_width);
        int grid_y = (9.0f - norm_y) / (18.0f / grid_height);

        if (norm_x > -9 && norm_x < 9 && norm_y > -9 && norm_y < 9 && grid_x < grid_width && grid_y < grid_height)
        {
            std::cout << "Clicked object x: " << grid_x << std::endl;
            std::cout << "Clicked object y: " << grid_y << std::endl;
            match_and_pop(grid_x, grid_y, true);
        }
    }
}
void moveObjectsDown(int i, int j)
{
    for (int k = j; k > 0; k--)
    {
        colorGrid[i][k] = colorGrid[i][k - 1];
        colorGrid[i][k].original_j = k;
        colorGrid[i][k].isMoving = true;
        lockPop = true;
    }
}
void updateObjectPosition()
{
    for (int i = 0; i < grid_width; i++)
    {
        for (int j = 0; j < grid_height; j++)
        {
            match_and_pop(i, j);
            if (colorGrid[i][j].isMoving == true)
            {
                colorGrid[i][j].yPos -= 0.4;
                if (colorGrid[i][j].yPos <= (10 - colorGrid[i][j].original_j * (18. / grid_height) - 1 - 18. / ((2) * (grid_height))))
                {
                    colorGrid[i][j].isMoving = false;
                    lockPop = false;
                }
            }
        }
    }
}
void addNewObject(int x, int offset)
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);
    float scaleFactor = min(1.0f * (5.0f / grid_width), 1.0f * (5.0f / grid_height)) / 2.;
    glm::vec3 color = colorArray[rand() % 5];
    colorGrid[x][0].color = color;
    colorGrid[x][0].yPos = 10 + offset * (18. / grid_height);
    colorGrid[x][0].isClicked = false;
    colorGrid[x][0].isMoving = true;
    colorGrid[x][0].scaleFactor = scaleFactor;
}

void display(GLFWwindow *window)
{
    updateObjectPosition();
    float scale = min(1.0f * (5.0f / grid_width), 1.0f * (5.0f / grid_height)) / 2;

    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    static float angle = 0;
    glUseProgram(gProgram[0]);

    for (int i = 0; i < grid_width; i++)
    {
        for (int j = 0; j < grid_height; j++)
        {
            if (colorGrid[i][j].isClicked == true && colorGrid[i][j].scaleFactor < (1.5 * scale))
            {
                lockPop = true;
                colorGrid[i][j].scaleFactor += 0.01;
            }
            if (colorGrid[i][j].isClicked == true && colorGrid[i][j].scaleFactor >= (1.5 * scale))
            {
                int offset = colorGrid[i][j].yOffset;
                moveObjectsDown(i, j);
                addNewObject(i, offset);
                colorGrid[i][j].isClicked = false;

                // move down the objects above the empty space
            }
            glm::vec3 light = glm::vec3((i) * (18. / grid_width) - 10 + 1 + (18. / ((2) * (grid_width))), colorGrid[i][j].yPos, 1.f);
            glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3((i) * (18. / grid_width) - 10 + 1 + (18. / ((2) * (grid_width))), colorGrid[i][j].yPos, -10.f));
            // glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3((i) * (18. / grid_width) - 10 + 1 + (18. / ((2) * (grid_width))), 10 - j * (18. / grid_height) - 1 - 18. / ((2) * (grid_height)), -10.f));
            glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
            glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(colorGrid[i][j].scaleFactor, colorGrid[i][j].scaleFactor, colorGrid[i][j].scaleFactor));
            glm::mat4 modelMat = T * R * S;
            glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
            glm::mat4 projectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
            glm::vec3 color = colorGrid[i][j].color;

            glUniform3f(glGetUniformLocation(gProgram[0], "lightPos"), light.x, light.y, light.z);
            glUniform3f(glGetUniformLocation(gProgram[0], "kd"), color.x, color.y, color.z);
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            if (colorGrid[i][j].show && !(colorGrid[i][j].isClicked && colorGrid[i][j].scaleFactor >= (1.5 * scale)))
                drawModel();
        }
    }

    assert(glGetError() == GL_NO_ERROR);

    std::string movesAndScore = std::string("Moves: ") + std::to_string(moves) + "    Score: " + std::to_string(score);
    renderText(movesAndScore, 0, 0, 1, glm::vec3(0, 1, 1));

    assert(glGetError() == GL_NO_ERROR);

    angle += 0.5;
}

void reshape(GLFWwindow *window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);
}

void constructColorArray()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);
    float scaleFactor = min(1.0f * (5.0f / grid_width), 1.0f * (5.0f / grid_height)) / 2.;
    std::vector<std::vector<Fistik>> temp(grid_width, std::vector<Fistik>(grid_height));
    for (int i = 0; i < grid_width; i++)
    {
        for (int j = 0; j < grid_height; j++)
        {
            glm::vec3 color = colorArray[rand() % 5];
            temp[i][j].color = color;
            temp[i][j].scaleFactor = scaleFactor;
            temp[i][j].yPos = 10 - j * (18. / grid_height) - 1 - 18. / ((2) * (grid_height));
        }
    }

    colorGrid = temp;
}

void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        cout << "R pressed" << endl;
        moves = 0;
        score = 0;
        constructColorArray();
    }
}

void mainLoop(GLFWwindow *window)
{
    while (!glfwWindowShouldClose(window))
    {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
void ParseCommandLineArguments(int argc, char *argv[], string &objFileName)
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << "gridWidth gridHeight obj" << endl;
        exit(1);
    }

    grid_width = atoi(argv[1]);
    grid_height = atoi(argv[2]);
    objFileName = argv[3];
}

int main(int argc, char **argv) // Create Main Function For Bringing It All Together
{

    string objFileName;
    ParseCommandLineArguments(argc, argv, objFileName);
    constructColorArray();
    GLFWwindow *window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char *)glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char *)glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init(objFileName);

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window);                 // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
