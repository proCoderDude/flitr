#include <flitr/modules/glsl_shader_passes/glsl_translate_pass.h>

using namespace flitr;

GLSLTranslatePass::GLSLTranslatePass(flitr::TextureRectangle *in_tex, bool read_back_to_CPU) :
    GLSLImageProcessor(in_tex, nullptr, read_back_to_CPU)
{
    TextureWidth_ = in_tex->getTextureWidth();
    TextureHeight_ = in_tex->getTextureHeight();

    RootGroup_ = new osg::Group;
    InTexture_ = in_tex;
    OutTexture_=0;
    OutImage_=0;
   
    createOutputTexture(read_back_to_CPU);

    Camera_ = new osg::Camera;
    
    setupCamera();

    UniformTranslation_=osg::ref_ptr<osg::Uniform>(new osg::Uniform("translation", osg::Vec2f(0.0f, 0.0f)));

    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

    setShader();
}

GLSLTranslatePass::~GLSLTranslatePass()
{
}

osg::ref_ptr<osg::Group> GLSLTranslatePass::createTexturedQuad()
{
    osg::ref_ptr<osg::Group> top_group = new osg::Group;
    
    osg::ref_ptr<osg::Geode> quad_geode = new osg::Geode;

    osg::ref_ptr<osg::Vec3Array> quad_coords = new osg::Vec3Array; // vertex coords
    // counter-clockwise
    quad_coords->push_back(osg::Vec3d(0, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 1, -1));
    quad_coords->push_back(osg::Vec3d(0, 1, -1));

    osg::ref_ptr<osg::Vec2Array> quad_tcoords = new osg::Vec2Array; // texture coords
    quad_tcoords->push_back(osg::Vec2(0, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, TextureHeight_));
    quad_tcoords->push_back(osg::Vec2(0, TextureHeight_));

    osg::ref_ptr<osg::Geometry> quad_geom = new osg::Geometry;
    osg::ref_ptr<osg::DrawArrays> quad_da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);

    osg::ref_ptr<osg::Vec4Array> quad_colors = new osg::Vec4Array;
    quad_colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    quad_geom->setVertexArray(quad_coords.get());
    quad_geom->setTexCoordArray(0, quad_tcoords.get());
    quad_geom->addPrimitiveSet(quad_da.get());
    quad_geom->setColorArray(quad_colors.get());
    quad_geom->setColorBinding(osg::Geometry::BIND_OVERALL);

    StateSet_ = quad_geom->getOrCreateStateSet();
    StateSet_->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    StateSet_->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
    StateSet_->setTextureAttributeAndModes(0, InTexture_.get(), osg::StateAttribute::ON);
    
    StateSet_->addUniform(new osg::Uniform("textureID0", 0));

    StateSet_->addUniform(UniformTranslation_);

    quad_geode->addDrawable(quad_geom.get());
    
    top_group->addChild(quad_geode.get());

    return top_group;
}

void GLSLTranslatePass::setupCamera()
{
    // clearing
    bool need_clear = false;
    if (need_clear) {
        Camera_->setClearColor(osg::Vec4(0.1f,0.1f,0.3f,1.0f));
        Camera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        Camera_->setClearMask(0);
        Camera_->setImplicitBufferAttachmentMask(0, 0);
    }

    // projection and view
    Camera_->setProjectionMatrixAsOrtho(0,1,0,1,-100,100);
    Camera_->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    Camera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    Camera_->setViewMatrix(osg::Matrix::identity());

    // viewport
    Camera_->setViewport(0, 0, TextureWidth_, TextureHeight_);

    Camera_->setRenderOrder(osg::Camera::PRE_RENDER);
    Camera_->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

	// attach the texture and use it as the color buffer.
	Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), OutTexture_.get());

	if (OutImage_!=0)
        {
	    Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), OutImage_.get());
        }
}

void GLSLTranslatePass::createOutputTexture(bool read_back_to_CPU)
{
    OutTexture_ = new flitr::TextureRectangle;

    OutTexture_->setTextureSize(TextureWidth_, TextureHeight_);
    OutTexture_->setInternalFormat(InTexture_->getInternalFormat());
    OutTexture_->setSourceFormat(InTexture_->getSourceFormat());
    OutTexture_->setSourceType(InTexture_->getSourceType());
    OutTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    OutTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

    if (read_back_to_CPU)
    {
        OutImage_ = new osg::Image;
        OutImage_->allocateImage(TextureWidth_, TextureHeight_, 1,
                                 InTexture_->getInternalFormat(), InTexture_->getInternalFormatType());
    }
}

void GLSLTranslatePass::setShader()
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT,
                                                         "uniform sampler2DRect textureID0;\n"
                                                         "uniform vec2 translation;\n"
                                                         "\n"
                                                         "void main(void)\n"
                                                         "{\n"
                                                         "    vec2 texCoord = gl_TexCoord[0].xy - translation;\n"
                                                         "    vec4 c  = texture2DRect(textureID0, texCoord);\n"
                                                         "    gl_FragColor = c;\n"
                                                         "}\n"
                                                         );

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;
    FragmentProgram_->setName("glsl_translate");
    FragmentProgram_->addShader(fshader.get());

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}

void GLSLTranslatePass::setTranslation(osg::Vec2f translation)
{
    UniformTranslation_->set(translation);
}

osg::Vec2f GLSLTranslatePass::getTranslation() const
{
    osg::Vec2f rValue;

    UniformTranslation_->get(rValue);

    return rValue;
}
