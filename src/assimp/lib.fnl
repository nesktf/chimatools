(local ffi (require :ffi))

(fn defined? [file-content define-name]
  (let [def-str (string.format "#define %s" define-name)
        filter (file-content:match def-str)]
    (not= nil filter)))

(fn parse-def [file-content define-name default-val]
  (let [target (string.format "#define %s (.-)\n" define-name)
        val-match (file-content:match target)]
    (if val-match
        val-match
        (tostring default-val))))

(fn parse-arr-sz [file-content define-name default-val]
  (let [define-val (parse-def file-content define-name default-val)]
    (string.format "static const unsigned int %s = %d;\n" define-name
                   define-val)))

;; fnlfmt: skip
(local assimp-conf-defs
 [(fn [file-content]
    (parse-arr-sz file-content "AI_MAX_NUMBER_OF_TEXTURECOORDS" 8))
  (fn [file-content]
    (parse-arr-sz file-content "AI_MAX_NUMBER_OF_COLOR_SETS" 8))
  (fn [file-content]
    (if (defined? file-content "ASSIMP_DOUBLE_PRECISION")
        "typedef double ai_real;\n"
        "typedef float ai_real;\n"))])

(fn gen-cdef-header [include-path]
  "Parse compile time definitions from assimp config header"
  (var header "")
  (let [path (.. include-path "config.h")
        file (assert (io.open path "r") "Assimp header %s not found" path)
        file-content (file:read "*all")]
    (each [_i parser (ipairs assimp-conf-defs)]
      (set header (.. header (parser file-content)))))
  header)

;; TODO: Don't hardcode the include folder?
(ffi.cdef (gen-cdef-header "/usr/include/assimp/"))

;; ASSIMP 5.2.5 C interface
(ffi.cdef "
  typedef struct aiScene aiScene;

  const aiScene* aiImportFile(const char*, unsigned int);
  void aiReleaseImport(const aiScene*);
  const char* aiGetErrorString();

  typedef uint32_t ai_uint32;
  typedef signed int ai_int;
  typedef unsigned int ai_uint;
  typedef unsigned char ai_byte;
  typedef double ai_double;
  typedef float ai_float;

  static const unsigned int AI_FORMAT_HINT_SIZE = 9;

  typedef struct aiVector2D {
    ai_real x, y;
  } aiVector2D;

  typedef struct aiVector3D {
    ai_real x, y, z;
  } aiVector3D;

  typedef struct aiColor4D {
    ai_real r, g, b, a;
  } aiColor4D;

  typedef struct aiColor3D {
    ai_real r, g, b;
  } aiColor3D;

  typedef struct aiMatrix3x3 {
    ai_real a1, a2, a3;
    ai_real b1, b2, b3;
    ai_real c1, c2, c3;
  } aiMatrix3x3;

  typedef struct aiMatrix4x4 {
    ai_real a1, a2, a3, a4;
    ai_real b1, b2, b3, b4;
    ai_real c1, c2, c3, c4;
    ai_real d1, d2, d3, d4;
  } aiMatrix4x4;

  typedef struct aiQuaternion {
    ai_real w, x, y, z;
  } aiQuaternion;

  typedef struct aiPlane {
    ai_real a, b, c, d;
  } aiPlane;

  typedef struct aiRay {
    aiVector3D pos, dir;
  } aiRay;

  typedef struct aiAABB {
    aiVector3D mMin;
    aiVector3D mMax;
  } aiAABB;


  typedef struct aiString {
    ai_uint32 length;
    char data[1024];
  } aiString;

  typedef struct aiTexel {
    ai_byte b, g, r, a;
  } aiTexel;


  typedef enum aiMetadataType {
    AI_BOOL = 0,
    AI_INT32 = 1,
    AI_UINT64 = 2,
    AI_FLOAT = 3,
    AI_DOUBLE = 4,
    AI_AISTRING = 5,
    AI_AIVECTOR3D = 6,
    AI_AIMETADATA = 7,
    AI_META_MAX = 8,

    FORCE_32BIT = 0x7fffffff
  } aiMetadataType;

  typedef struct aiMetadataEntry {
    aiMetadataType mType;
    void* mData;
  } aiMetadataEntry;

  typedef struct aiMetadata {
    ai_uint mNumProperties;
    aiString* mKeys;
    aiMetadataEntry* mValues;
  } aiMetadata;

  typedef struct aiNode aiNode;
  typedef struct aiNode {
    aiString mName;
    aiMatrix4x4 mTransformation;
    aiNode* mParent;

    ai_uint mNumChildren;
    aiNode** mChildren;

    ai_uint mNumMeshes;
    ai_uint* mMeshes;

    aiMetadata* mMetaData;
  } aiNode;


  typedef struct aiMesh aiMesh;
  typedef struct aiVertexWeight {
    ai_uint mVertexId;
    ai_real mWeight;
  } aiVertexWeight;

  typedef struct aiSkeletonBone {
    ai_int mParent;
    aiNode* mArmature;
    aiNode* mNode;
    ai_uint mNumnWeigths;
    aiMesh* mMeshId;
    aiVertexWeight* mWeights;
    aiMatrix4x4 mOffsetMatrix;
    aiMatrix4x4 mLocalMatrix;
  } aiSkeletonBone;

  typedef struct aiSkeleton {
    aiString mName;
    ai_uint mNumBones;
    aiSkeletonBone** mBones;
  } aiSkeleton;

  typedef struct aiBone {
    aiString mName;
    ai_uint mNumWeights;
    aiNode* mArmature;
    aiNode* mNode;
    aiVertexWeight* mWeights;
    aiMatrix4x4 mOffsetMatrix;
  } aiBone;


  typedef struct aiAnimMesh {
    aiString mName;

    aiVector3D* mVertices;
    aiVector3D* mNormals;
    aiVector3D* mTangents;
    aiVector3D* mBitangents;
    aiColor4D* mColors[AI_MAX_NUMBER_OF_COLOR_SETS];
    aiVector3D* mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    ai_uint mNumVertices;
    ai_float mWeight;
  } aiAnimMesh;

  typedef struct aiFace {
    ai_uint mNumIndices;
    ai_uint* mIndices;
  } aiFace;

  typedef struct aiMesh {
    ai_uint mPrimitiveTypes;
    ai_uint mNumVertices;
    ai_uint mNumFaces;

    aiVector3D* mVertices;
    aiVector3D* mNormals;
    aiVector3D* mTangents;
    aiVector3D* mBitangents;

    aiColor4D* mColors[AI_MAX_NUMBER_OF_COLOR_SETS];
    aiVector3D* mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
    ai_uint mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];
    aiFace* mFaces;

    ai_uint mNumBones;
    aiBone** mBones;

    ai_uint mMaterialIndex;

    aiString mName;

    ai_uint mNumAnimMeshes;
    aiAnimMesh** mAnimMeshes;

    ai_uint mMethod;
    aiAABB mAABB;

    aiString** mTextureCoordsNames;
  } aiMesh;


  typedef enum aiPropertyTypeInfo {
    aiPTI_Float = 0x1,
    aiPTI_Double = 0x2,
    aiPTI_String = 0x3,
    aiPTI_Integer = 0x4,
    aiPTI_Buffer = 0x5,

    _aiPTY_Force32Bit = 0x7fffffff,
  } aiPropertyTypeInfo;

  typedef struct aiMaterialProperty {
    aiString mKey;
    ai_uint mSemantic;
    ai_uint mIndex;
    ai_uint mDataLength;
    aiPropertyTypeInfo mType;
    char* mData;
  } aiMaterialProperty;

  typedef struct aiMaterial {
    aiMaterialProperty** mProperties;
    ai_uint mNumProperties;
    ai_uint mNumAllocated;
  } aiMaterial;


  typedef struct aiVectorKey {
    ai_double mTime;
    aiVector3D mValue;
  } aiVectorKey;

  typedef struct aiQuatKey {
    ai_double mTime;
    aiQuaternion mValue;
  } aiQuatKey;

  typedef enum aiAnimBehaviour {
    aiAnimBehaviour_DEFAULT = 0x0,
    aiAnimBehaviour_CONSTANT = 0x1,
    aiAnimBehaviour_LINEAR = 0x2,
    aiAnimBehaviour_REPEAT = 0x3,

    _aiAnimBehaviour_Force32Bit = 0x7fffffff,
  } aiAnimBehaviour;

  typedef struct aiNodeAnim {
    aiString mNodeName;

    ai_uint mNumPositionKeys;
    aiVectorKey* mPositionKeys;
    aiQuatKey* mRotationKeys;
    aiVectorKey* mScalingKeys;

    aiAnimBehaviour mPreState;
    aiAnimBehaviour mPostState;
  } aiNodeAnim;

  typedef struct aiMeshKey {
    ai_double mTime;
    ai_uint mValue;
  } aiMeshKey;

  typedef struct aiMeshAnim {
    aiString mName;
    ai_uint mNumKeys;
    aiMeshKey* mKeys;
  } aiMeshAnim;

  typedef struct aiMeshMorphKey {
    ai_double mTime;
    ai_uint* mValues;
    ai_double* mWeights;
    ai_uint mNumValuesAndWeights;
  } aiMeshMorphKey;

  typedef struct aiMeshMorphAnim {
    aiString mName;
    ai_uint mNumKeys;
    aiMeshMorphKey* mKeys;
  } aiMeshMorphAnim;

  typedef struct aiAnimation {
    aiString mName;
    ai_double mDuration;
    ai_double mTicksPerSecond;

    ai_uint mNumChannels;
    aiNodeAnim** mChannels;

    ai_uint mNumMeshChannels;
    aiMeshAnim** mMeshChannels;

    ai_uint mNumMorphMeshChannels;
    aiMeshMorphAnim** mMorphMeshChannels;
  } aiAnimation;


  typedef struct aiTexture {
    ai_uint mWidth;
    ai_uint mHeight;

    char achFormatHint[AI_FORMAT_HINT_SIZE];
    aiTexel* pcData;
    aiString mFilename;
  } aiTexture;


  typedef enum aiLightSourceType {
    aiLightSource_UNDEFINED     = 0x0,
    aiLightSource_DIRECTIONAL   = 0x1,
    aiLightSource_POINT         = 0x2,
    aiLightSource_SPOT          = 0x3,
    aiLightSource_AMBIENT       = 0x4,
    aiLightSource_AREA          = 0x5,

    _aiLightSource_Force32Bit   = 0x7fffffff,
  } aiLightSourceType;

  typedef struct aiLight {
    aiString mName;
    aiLightSourceType mType;

    aiVector3D mPosition;
    aiVector3D mDirection;
    aiVector3D mUp;

    ai_float mAttenuationConstant;
    ai_float mAttenuationLinear;
    ai_float mAttenuationQuadratic;

    aiColor3D mColoDiffuse;
    aiColor3D mColorSpecular;
    aiColor3D mColorAmbient;

    ai_float mAngleInnerCore;
    ai_float mAngleOuterCone;

    aiVector2D mSize;
  } aiLight;


  typedef struct aiCamera {
    aiString mName;

    aiVector3D mPosition;
    aiVector3D mUp;
    aiVector3D mLookAt;

    ai_float mHorizontalFOV;
    ai_float mClipPlaneNear;
    ai_float mClipPlaneFar;
    ai_float mAspect;
    ai_float mOrthographicWidth;
  } aiCamera;


  typedef struct aiScene {
    ai_uint mFlags;
    aiNode* mRootNode;

    ai_uint mNumMeshes;
    aiMesh** mMeshes;

    ai_uint mNumMaterials;
    aiMaterial** mMaterials;

    ai_uint mNumAnimations;
    aiAnimation** mAnimations;

    ai_uint mNumTextures;
    aiTexture** mTextures;

    ai_uint mNumLights;
    aiLight** mLights;

    ai_uint mNumCameras;
    aiCamera** mCameras;

    aiMetadata* mMetadata;
    aiString mName;

    ai_uint mNumSkeletons;
    aiSkeleton** mSkeletons;

    char* mPrivate;
  } aiScene;
")

{:lib (ffi.load :assimp)}
