#include "Angel.h"
#include <assert.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

typedef Angel::vec4  color4;
typedef Angel::vec4  point4;

enum {
    Sun = 0,
    Mercury = 1,
    Venus = 2,
    Earth = 3,
    Mars = 4,
    Jupiter = 5,
    Saturn = 6,
    Uranus = 7,
    Neptune = 8,
    Star = 9
};

float distance[9] =
{
    0,
    0.190110141210896,
    0.203211783465667,
    0.217075175486183,
    0.232001775410564,
    0.283225657970443,
    0.417125150005754,
    0.592683335799181,
    0.868979632095478
};

float rotation[9] =
{
    //TODO: kendi eksenlerinde eğimli olunca shading yanlış oluyor
    0,0,0,0,0,0,0,0,0
//    0,
//    0,
//    177.36,
//    23.45,
//    25.19,
//    3.13,
//    26.73,
//    97.77,
//    28.32
};

float radius[9] =
{
    0.8,
    0.00935154041492981,
    0.0231969718748503,
    0.0244477025537828,
    0.0130178716879881,
    0.204033826841071,
    0.181011451296057,
    0.097969431268267,
    0.0949221407694888
};

float speed[9] = {
    0,
    12.453300124533,
    4.87012987012987,
    3.0,
    1.57894736842105,
    0.3,
    0.101694915254237,
    0.0357142857142857,
    0.0181818181818182
};

float aellipse[9] =
{
    0,
    57.9,
    108,
    150,
    228,
    779,
    1430,
    2870,
    4500
};

float bellipse[9] =
{
    0,
    56.6703,
    107.9974,
    149.9783,
    226.9905,
    778.0643,
    488.1149,
    2866.9619,
    4499.7277
};

float x[9] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

float y[9] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int Axis = Xaxis;
GLfloat  Theta[NumAxes] = { 20.0, 20.0, 0.0 };
bool isSun = true;

// Model-view and projection matrices uniform location
GLuint  GlobalModelView, ModelView, Projection, ShadingMethod, LightSources, Shininess, Distance, IsSun, IsStar, Color, StarColor, Transparency;

int Index = 0;

//----------------------------------------------------------------------------
const int TextureSize = 64;
float programMode = 1;
const int NumTimesToSubdivide = 5;
const int NumTriangles = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int SphereNumVertices = 3 * NumTriangles;
const int NumNodes = 9;
float currentColor = 0;
GLuint textures[2];
GLubyte image[TextureSize][TextureSize][3];
GLubyte image2[TextureSize][TextureSize][3];
vec2 tex_coords[SphereNumVertices];

point4 SphereVertices[SphereNumVertices];
point4 SphereColors[SphereNumVertices];
vec3 SphereNormals[SphereNumVertices];

int SphereIndex = 0;

point4
unit( const point4& p )
{
    float len = p.x* p.x + p.y * p.y + p.z * p.z;
    
    point4 t;
    if ( len > DivideByZeroTolerance ) {
        t = p / sqrt(len);
        t.w = 5;
    }
    return t;
}

int texX = 0;
int texY = 0;

void
triangle( const point4& a, const point4& b, const point4& c )
{
    vec4 u = b - a;
    vec4 v = c - a;
    vec3 normal = normalize( cross(u, v) );
    SphereNormals[SphereIndex] = normal;  SphereVertices[SphereIndex] = a;
    tex_coords[SphereIndex] = vec2(0, 0.0);
    texY+=1;
    if(texY==5) { texX ++; }
    SphereIndex++;
    SphereNormals[SphereIndex] = normal;  SphereVertices[SphereIndex] = b;
    tex_coords[SphereIndex] = vec2(0.5,1.0);
    texY+=1;
    if(texY==5) { texX ++; }
    SphereIndex++;
    SphereNormals[SphereIndex] = normal;  SphereVertices[SphereIndex] = c;
    tex_coords[SphereIndex] = vec2(1.0,1.0);
    texY+=1;
    if(texY==5) { texX ++; }
    SphereIndex++;
}

void
divide_triangle( const point4& a, const point4& b,
                const point4& c, int count )
{
    if ( count > 0 ) {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle(  a, v1, v2, count - 1 );
        divide_triangle(  c, v2, v3, count - 1 );
        divide_triangle(  b, v3, v1, count - 1 );
        divide_triangle( v1, v3, v2, count - 1 );
    }
    else {
        triangle( a, b, c );
    }
}

void
tetrahedron( int count )
{
    point4 v[4] = {
        vec4( 0.0, 0.0, 1.0, 100.0 ),
        vec4( 0.0, 0.942809, -0.333333, 100.0 ),
        vec4( -0.816497, -0.471405, -0.333333, 100.0 ),
        vec4( 0.816497, -0.471405, -0.333333, 100.0 )
    };
    
    divide_triangle( v[0], v[1], v[2], count );
    divide_triangle( v[3], v[2], v[1], count );
    divide_triangle( v[0], v[3], v[1], count );
    divide_triangle( v[0], v[2], v[3], count );
}

//----------------------------------------------------------------------------

struct Node {
    mat4 transform;
    void (*render)( void );
    Node* sibling;
    Node* child;
    
    Node() :
    render(NULL), sibling(NULL), child(NULL) {}
    
    Node( mat4& m, void (*render)( void ), Node* sibling, Node* child ) :
    transform(m), render(render), sibling(sibling), child(child) {}
};

Node nodes[NumNodes];
Node starnodes[10];

//----------------------------------------------------------------------------

class MatrixStack {
    int    index;
    int    size;
    mat4*  matrices;
    
public:
    MatrixStack( int numMatrices = 32 ):index(0), size(numMatrices)
    { matrices = new mat4[numMatrices]; }
    
    ~MatrixStack()
    { delete[] matrices; }
    
    void push( const mat4& m ) {
        assert( index + 1 < size );
        matrices[index++] = m;
    }
    
    mat4& pop( void ) {
        assert( index - 1 >= 0 );
        index--;
        return matrices[index];
    }
};

MatrixStack  mvstack;

//----------------------------------------------------------------------------

mat4 general_model_view;
mat4 model_view;

void
traverse( Node* node )
{
    if ( node == NULL ) { return; }
    
    mvstack.push( model_view );
    
    model_view = general_model_view * node->transform;
    node->render();
    
    traverse( node->child );
    if ( node->child != NULL) { traverse( node->child ); }
    
    model_view = mvstack.pop();
    
    traverse( node->sibling );
    if ( node->sibling != NULL) { traverse( node->sibling ); }
}

//----------------------------------------------------------------------------

void
sun_draw()
{
    mat4 instance = ( RotateZ(rotation[Sun]) *
                     Scale( radius[Sun], radius[Sun], radius[Sun] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform4f(Color, 0.8, 0.7, 0.0, 1.0);
    glUniform1f(IsSun, 1);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
    
}

void
mercury_draw()
{
    mat4 instance = ( RotateZ(rotation[Mercury]) *
                     Scale( radius[Mercury], radius[Mercury], radius[Mercury] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
venus_draw()
{
    mat4 instance = (  RotateZ(rotation[Venus]) *
                     Scale( radius[Venus], radius[Venus], radius[Venus] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
earth_draw()
{
    mat4 instance = ( RotateZ(rotation[Earth]) *
                     Scale( radius[Earth], radius[Earth], radius[Earth] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
mars_draw()
{
    mat4 instance = ( RotateZ(rotation[Mars]) *
                     Scale( radius[Mars], radius[Mars], radius[Mars] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
jupiter_draw()
{
    mat4 instance = ( RotateZ(rotation[Jupiter]) *
                     Scale( radius[Jupiter], radius[Jupiter], radius[Jupiter] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
saturn_draw()
{
    mat4 instance = ( RotateZ(rotation[Saturn]) *
                     Scale( radius[Saturn], radius[Saturn], radius[Saturn] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
uranus_draw()
{
    mat4 instance = ( RotateZ(rotation[Uranus]) *
                     Scale( radius[Uranus], radius[Uranus], radius[Uranus] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
neptune_draw()
{
    mat4 instance = ( RotateZ(rotation[Neptune]) *
                     Scale( radius[Neptune], radius[Neptune], radius[Neptune] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 0);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

void
stars_draw()
{
    mat4 instance = (Scale( 0.01, 0.01, 0.01 ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    //glUniform4f(StarColor, 1.0, 1.0, 1.0, 1.0);
    glUniform1f(IsSun, 0);
    glUniform1f(IsStar, 1);
    glDrawArrays( GL_TRIANGLES, 0, SphereNumVertices );
}

//--------------------------------------------------------------------

void
initNodes( void )
{
    mat4 m;
    
    m = Translate( distance[Sun], 0.0, 0.0 );
    nodes[Sun] = Node( m, sun_draw, NULL, &nodes[Mercury] );
    
    m = Translate( distance [Mercury], 0.0, 0.0 );
    nodes[Mercury] = Node( m, mercury_draw, &nodes[Venus], NULL );
    
    m = Translate( distance[Venus], 0.0, 0.0 );
    nodes[Venus] = Node( m, venus_draw, &nodes[Earth], NULL );
    
    m = Translate( distance[Earth], 0.0, 0.0 );
    nodes[Earth] = Node( m, earth_draw, &nodes[Mars], NULL );
    
    m = Translate( distance[Mars], 0.0, 0.0 );
    nodes[Mars] = Node( m, mars_draw, &nodes[Jupiter], NULL );
    
    m = Translate( distance[Jupiter], 0.0, 0.0 );
    nodes[Jupiter] = Node( m, jupiter_draw, &nodes[Saturn], NULL );
    
    m = Translate( distance[Saturn], 0.0, 0.0 );
    nodes[Saturn] = Node( m, saturn_draw, &nodes[Uranus], NULL );
    
    m = Translate( distance[Uranus], 0.0, 0.0 );
    nodes[Uranus] = Node( m, uranus_draw, &nodes[Neptune], NULL );
    
    m = Translate( distance[Neptune], 0.0, 0.0 );
    nodes[Neptune] = Node( m, neptune_draw, NULL, NULL );
    
}

void initStarNodes( void ) {
    mat4 m;
    
    double starX = ((double)rand() / (double)(RAND_MAX));
    double starY = ((double)rand() / (double)(RAND_MAX));
    double starZ = ((double)rand() / (double)(RAND_MAX));
    m = Translate( starX, starY, starZ );
    starnodes[0] = Node( m, stars_draw, NULL, &starnodes[1] );
    for(int i=1; i<10; i++){
        double starX = ((double)rand() / (double)(RAND_MAX));
        double starY = ((double)rand() / (double)(RAND_MAX));
        double starZ = ((double)rand() / (double)(RAND_MAX));
        if(starX > 0.5){
        m = Translate( -starX, starY, starZ );
        }
        if(starZ < 0.2){
            m = Translate( -starX, starY, starZ );
        }
        if(starZ < 0.5){
            m = Translate( -starX, starY, -starZ );
        }
        if(starZ < 0.8){
            m = Translate( starX, -starY, starZ );
        }
        if(starZ < 1){
            m = Translate( -starX, -starY, -starZ );
        }
        if(starY > 0.5){
            m = Translate( starX, starY, starZ );
        }
        
        if(i<9){
            starnodes[i] = Node( m, stars_draw, &starnodes[i+1], NULL );
        } else {
            starnodes[i] = Node( m, stars_draw, NULL, NULL );
        }
    }
}

//--------------------------------------------------------------------

// initialization is made according to the program mode
void
init()
{
    SphereIndex = 0;
    tetrahedron(NumTimesToSubdivide);
    
    initNodes();
    initStarNodes();
    traverse(&starnodes[0]);
    
    // Initialize texture objects
    glGenTextures( 2, textures );
    
    glBindTexture( GL_TEXTURE_2D, textures[0] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, TextureSize, TextureSize, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image );
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR); //try here different alternatives
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //try here different alternatives
    
    glBindTexture( GL_TEXTURE_2D, textures[1] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, TextureSize, TextureSize, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image2 );
    //glGenerateMipmap(GL_TEXTURE_2D); // try also activating mipmaps for the second texture object
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
    
    glBindTexture( GL_TEXTURE_2D, textures[0] ); //set current texture
    
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(SphereVertices) + sizeof(SphereNormals) + sizeof(tex_coords),
                 NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(SphereVertices), SphereVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(SphereVertices),
                    sizeof(SphereNormals), SphereNormals );
     glBufferSubData( GL_ARRAY_BUFFER, sizeof(SphereVertices) + sizeof(SphereNormals), sizeof(tex_coords), tex_coords );
    
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );
    
    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0) );
    
    GLuint vNormal = glGetAttribLocation( program, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(SphereVertices)) );
    
    GLuint vTexCoord = glGetAttribLocation( program, "vTexCoord" );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(SphereVertices) + sizeof(SphereNormals)) );
    
    // Light Source Properties
    point4 light1_position( -0.1, 0.0, 0.0, 0.0 );
    color4 light1_ambient( 1.0, 1.0, 1.0, 1.0 ); // L_a
    color4 light1_diffuse( 1.0, 1.0, 1.0, 1.0 ); // L_d
    color4 light1_specular( 1.0, 1.0, 1.0, 1.0 ); // L_s
    
    // Light Source Properties
    color4 material_ambient( 0.1, 0.7, 1.0, 1.0 ); // k_a
    color4 material_diffuse( 0.0, 0.8, 1.0, 1.0 ); // k_d
    color4 material_specular( 0.1, 0.7, 1.0, 1.0 ); // k_s
    
    color4 ambient_product1 = light1_ambient * material_ambient; // k_a * L_a
    color4 diffuse_product1 = light1_diffuse * material_diffuse; // k_d * L_d
    color4 specular_product1 = light1_specular * material_specular; // k_s * L_s
    
    glUniform4fv( glGetUniformLocation(program, "AmbientProduct"),
                 1, ambient_product1 );
    glUniform4fv( glGetUniformLocation(program, "DiffuseProduct1"),
                 1, diffuse_product1 );
    glUniform4fv( glGetUniformLocation(program, "SpecularProduct1"),
                 1, specular_product1 );
    
    glUniform4fv( glGetUniformLocation(program, "LightPosition1"),
                 1, light1_position );
    
    Shininess = glGetUniformLocation(program, "Shininess");
    glUniform1f( Shininess, 5 );
    IsSun = glGetUniformLocation(program, "Sun");
    IsStar = glGetUniformLocation(program, "Star");
    StarColor = glGetUniformLocation(program, "StarColor");
    Transparency = glGetUniformLocation(program, "Transparency");
    
    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation( program, "ModelView" );
    GlobalModelView = glGetUniformLocation( program, "GlobalModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
    Color = glGetUniformLocation( program, "Color" );
    
    const vec3 viewer_pos( 0.0, 0.0, 2.0 );
    general_model_view = ( Translate( -viewer_pos ) *
                  RotateX( Theta[Xaxis] ) *
                  RotateY( Theta[Yaxis] ) *
                  RotateZ( Theta[Zaxis] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, general_model_view );
    glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
    
    mat4  projection;
    projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
    
    glEnable( GL_DEPTH_TEST );
    glEnable(GL_CULL_FACE);
    
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    
}
//----------------------------------------------------------------------------
bool flag = false;

void display( void )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    traverse( &nodes[Sun] );
    traverse( &starnodes[0] );
    glutSwapBuffers();
}

//----------------------------------------------------------------------------

int quartile[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
float speedFactor = 1;

void
idle( void )
{
    for(int i = 1; i < 9; i++){
        nodes[i].transform = RotateY(speed[i]*speedFactor) * nodes[i].transform;
    }
    if(flag)
    {
        currentColor += 0.05;
    }
    
    else
    {
        currentColor -= 0.05;
    }
    
    if(currentColor > 0.95)
    {
        flag = false;
    }
    
    else if (currentColor < 0.05)
    {
        flag = true;
    }
    glUniform4f(StarColor, currentColor, currentColor, currentColor, 1.0);
    glutPostRedisplay();
}

//---------------------------------------------------------------------
//
// creating and functioning menus
//
FILE *fd;
int k, n, m, nm;
char c;
int i;
char b[100];
float s;
int red, green, blue;
int* images;
GLubyte mercury_texture[1024][512][3];
GLubyte venus_texture[1024][512][3];
GLubyte earth_texture[1000][500][3];
GLubyte mars_texture[1024][512][3];
GLubyte jupiter_texture[1024][512][3];
GLubyte saturn_texture[1800][900][3];
GLubyte uranus_texture[1024][512][3];
GLubyte neptune_texture[1024][512][3];

void fillTextures(char* dir)
{
    system ("ls");
    fd = fopen(dir, "r");
    fscanf(fd,"%[^\n] ",b);
    if(b[0]!='P'|| b[1] != '3'){
        printf("%s is not a PPM file!\n", b);
        exit(0); }    printf("%s is a PPM file \n",b);
    fscanf(fd, "%c",&c);
    while(c == '#')
    {
        fscanf(fd, "%[^\n] ", b);
        printf("%s\n",b);
        fscanf(fd, "%c",&c);
    }
    ungetc(c,fd);
    fscanf(fd, "%d %d %d", &n, &m, &k);
    printf("%d rows %d columns max value= %d\n",n,m,k);
    nm = n*m;
    images = (int*)malloc(3*sizeof(GLuint)*nm);
    for(i=nm;i>0;i--)
    {
        fscanf(fd,"%d %d %d",&red, &green, &blue );
        images[3*nm-3*i]=red;
        images[3*nm-3*i+1]=green;
        images[3*nm-3*i+2]=blue;
    }
    GLubyte tex_image[n][m][3];
    int counter = 0;
    for(i=0; i<n; i++){
        for(int j=0; j<m; j++){
            for(int k=0; k<3; k++){
                tex_image[i][j][k] = images[counter];
                counter++;
            }
        }
    }
    for(i=0; i<nm; i++){
        std::cout <<" for i = " << i << " " << images[i] << std::endl;
    }

}

void
myMenu(int id)
{
    
}

void
createMenu()
{
    
}

//The distance of the point source is changed through a variable and sent through the shader formula.
void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
        case 033: // Escape Key
        case 'q': case 'Q':
            exit( EXIT_SUCCESS );
            break;
        case 'z':
            general_model_view = Translate(0, 0, -0.02) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'x':
            general_model_view = Translate(0, 0, 0.02) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 's':
            general_model_view = Translate(0, 0.02, 0) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'w':
            general_model_view = Translate(0, -0.02, 0) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'a':
            general_model_view = Translate(0.02, 0, 0) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'd':
            general_model_view = Translate(-0.02, 0, 0) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'i':
            initNodes();
            glutPostRedisplay();
            break;
        case 'o':
            general_model_view = RotateZ(10) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'p':
            general_model_view = RotateZ(-10) * general_model_view;
            glUniformMatrix4fv( GlobalModelView, 1, GL_TRUE, general_model_view );
            traverse( &nodes[Sun] );
            glutPostRedisplay();
            break;
        case 'n':
            speedFactor += 0.1;
            glutPostRedisplay();
            break;
        case 'm':
            speedFactor -= 0.1;
            glutPostRedisplay();
            break;
        case 'c':
            speedFactor += 3;
            glutPostRedisplay();
            break;
        case 'v':
            speedFactor = 1;
            glutPostRedisplay();
            break;
    }
}

//----------------------------------------------------------------------------

void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );
    
    GLfloat aspect = GLfloat(width)/height;
    mat4 projection = Perspective( 45.0, aspect, 0.5, 3.0 );
    
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
}

//----------------------------------------------------------------------------

int
main( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( 800, 800 );
    glutCreateWindow( "Solar System" );
    
 /*   fillTextures("sun.ppm");
    fillTextures("mercury.ppm");
    fillTextures("venus.ppm");
    fillTextures("earth.ppm");
    fillTextures("mars.ppm");
    fillTextures("jupiter.ppm");
    fillTextures("saturn.ppm");
    fillTextures("uranus.ppm");
    fillTextures("neptune.ppm"); */
    
    init();
    
    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutReshapeFunc( reshape );

    glutIdleFunc( idle );
    createMenu();
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutMainLoop();
    return 0;
}
