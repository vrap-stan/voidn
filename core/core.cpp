#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

// stl
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// 3rd-party
#include <fbxsdk.h>
#include <draco/compression/encode.h>
#include <draco/core/encoder_buffer.h>
#include <draco/mesh/mesh.h>

#include "ktx_main.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageDenoise/oidn.hpp>

#include "core/core.h"

namespace OIIO = OpenImageIO_v3_0;

// #include <ktx.h>

// Initialize FBX SDK
void InitializeSdkObjects(FbxManager *&pManager, FbxScene *&pScene)
{
    pManager = FbxManager::Create();
    pManager->SetIOSettings(FbxIOSettings::Create(pManager, IOSROOT));
    pScene = FbxScene::Create(pManager, "Scene");
}

// Load FBX file
bool LoadScene(FbxManager *pManager, FbxScene *pScene, const char *pFilename)
{
    FbxImporter *lImporter = FbxImporter::Create(pManager, "");
    bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    if (!lImportStatus)
    {
        std::cerr << "Failed to initialize importer: " << lImporter->GetStatus().GetErrorString() << std::endl;
        return false;
    }
    bool lResult = lImporter->Import(pScene);
    lImporter->Destroy();
    return lResult;
}

// Save Draco file
bool SaveDracoFile(const std::string &filename, draco::EncoderBuffer &buffer)
{
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return false;
    }
    outFile.write(buffer.data(), buffer.size());
    outFile.close();
    return true;
}

// Process a single mesh and export to Draco
void ProcessMesh(FbxMesh *pMesh, const std::string &outputPrefix, int meshIndex)
{
    // Get control points (vertices)
    int controlPointCount = pMesh->GetControlPointsCount();
    if (controlPointCount == 0)
    {
        std::cerr << "Mesh " << meshIndex << " has no control points." << std::endl;
        return;
    }
    FbxVector4 *controlPoints = pMesh->GetControlPoints();

    // Create Draco mesh
    std::unique_ptr<draco::Mesh> dracoMesh = std::make_unique<draco::Mesh>();
    dracoMesh->set_num_points(controlPointCount);

    // Add positions
    std::vector<float> positions(controlPointCount * 3);
    for (int i = 0; i < controlPointCount; ++i)
    {
        positions[i * 3] = static_cast<float>(controlPoints[i][0]);
        positions[i * 3 + 1] = static_cast<float>(controlPoints[i][1]);
        positions[i * 3 + 2] = static_cast<float>(controlPoints[i][2]);
    }
    draco::GeometryAttribute posAttr;
    posAttr.Init(draco::GeometryAttribute::POSITION, nullptr, 3, draco::DT_FLOAT32, false, sizeof(float) * 3, 0);
    int posAttrId = dracoMesh->AddAttribute(posAttr, true, controlPointCount);
    draco::PointAttribute *posAttribute = dracoMesh->attribute(posAttrId);
    for (int i = 0; i < controlPointCount; ++i)
    {
        posAttribute->SetAttributeValue(draco::AttributeValueIndex(i), &positions[i * 3]);
    }

    // Add UVs (multiple channels)
    std::vector<int> uvAttrIds;
    int uvChannelCount = pMesh->GetElementUVCount();
    for (int uvChannel = 0; uvChannel < uvChannelCount; ++uvChannel)
    {
        FbxGeometryElementUV *uvElement = pMesh->GetElementUV(uvChannel);
        if (!uvElement)
            continue;

        std::vector<float> uvs;
        int uvCount = controlPointCount; // Default to control point count
        if (uvElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        {
            uvs.resize(controlPointCount * 2);
            for (int i = 0; i < controlPointCount; ++i)
            {
                FbxVector2 uv = uvElement->GetDirectArray().GetAt(i);
                uvs[i * 2] = static_cast<float>(uv[0]);
                uvs[i * 2 + 1] = static_cast<float>(uv[1]);
            }
        }
        else if (uvElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            uvCount = uvElement->GetDirectArray().GetCount();
            uvs.resize(uvCount * 2);
            for (int i = 0; i < uvCount; ++i)
            {
                FbxVector2 uv = uvElement->GetDirectArray().GetAt(i);
                uvs[i * 2] = static_cast<float>(uv[0]);
                uvs[i * 2 + 1] = static_cast<float>(uv[1]);
            }
        }

        draco::GeometryAttribute uvAttr;
        uvAttr.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2, draco::DT_FLOAT32, false, sizeof(float) * 2, 0);
        int uvAttrId = dracoMesh->AddAttribute(uvAttr, true, uvCount);
        draco::PointAttribute *uvAttribute = dracoMesh->attribute(uvAttrId);
        for (int i = 0; i < uvCount; ++i)
        {
            uvAttribute->SetAttributeValue(draco::AttributeValueIndex(i), &uvs[i * 2]);
        }
        uvAttrIds.push_back(uvAttrId);

        // Add UV set name as metadata
        std::string uvSetName = uvElement->GetName();
        if (uvSetName.empty())
            uvSetName = "uv" + std::to_string(uvChannel);
        auto metadata = std::make_unique<draco::AttributeMetadata>();
        metadata->AddEntryString("name", uvSetName);
        dracoMesh->AddAttributeMetadata(uvAttrId, std::move(metadata));
    }

    // Add faces (triangle indices)
    int polygonCount = pMesh->GetPolygonCount();
    std::vector<draco::Mesh::Face> faces;
    for (int poly = 0; poly < polygonCount; ++poly)
    {
        int polySize = pMesh->GetPolygonSize(poly);
        if (polySize != 3)
            continue; // Draco only supports triangles
        draco::Mesh::Face face;
        for (int vert = 0; vert < 3; ++vert)
        {
            face[vert] = draco::PointIndex(pMesh->GetPolygonVertex(poly, vert));
        }
        faces.push_back(face);
    }
    dracoMesh->SetNumFaces(faces.size());
    for (size_t i = 0; i < faces.size(); ++i)
    {
        dracoMesh->SetFace(draco::FaceIndex(static_cast<uint32_t>(i)), faces[i]);
    }

    // Encode to Draco
    draco::Encoder encoder;
    encoder.SetSpeedOptions(5, 5); // Moderate compression

    draco::EncoderBuffer buffer;
    if (!encoder.EncodeMeshToBuffer(*dracoMesh, &buffer).ok())
    {
        std::cerr << "Failed to encode mesh " << meshIndex << std::endl;
        return;
    }

    // Save to .drc file
    std::string outputFile = outputPrefix + "_mesh" + std::to_string(meshIndex) + ".drc";
    if (SaveDracoFile(outputFile, buffer))
    {
        std::cout << "Exported: " << outputFile << std::endl;
    }
}

// Traverse scene to find meshes
void ProcessNode(FbxNode *pNode, const std::string &outputPrefix, int &meshIndex)
{
    if (pNode->GetNodeAttribute() && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        FbxMesh *mesh = pNode->GetMesh();
        if (mesh)
        {
            ProcessMesh(mesh, outputPrefix, meshIndex++);
        }
    }
    for (int i = 0; i < pNode->GetChildCount(); ++i)
    {
        ProcessNode(pNode->GetChild(i), outputPrefix, meshIndex);
    }
}

extern "C"
{

    VCPP_API int vcpp_ktx(int argc, char **argv)
    {
        return ktx_main(argc, argv);
    }

    VCPP_API int vcpp_fbx(int argc, char *argv[])
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <input.fbx> <output_prefix>" << std::endl;
            return 1;
        }

        const char *inputFile = argv[1];
        std::string outputPrefix = argv[2];

        // Initialize FBX SDK
        FbxManager *lManager = nullptr;
        FbxScene *lScene = nullptr;
        InitializeSdkObjects(lManager, lScene);

        // Load FBX file
        if (!LoadScene(lManager, lScene, inputFile))
        {
            std::cerr << "Failed to load FBX file: " << inputFile << std::endl;
            lManager->Destroy();
            return 1;
        }

        // Process meshes
        int meshIndex = 0;
        ProcessNode(lScene->GetRootNode(), outputPrefix, meshIndex);

        // Cleanup
        lManager->Destroy();
        std::cout << "Processed " << meshIndex << " meshes." << std::endl;
        return 0;
    }

    VCPP_API int vcpp_image(int argc, char **file_name)
    {
        if (argc != 2)
        {
            std::cerr << "Usage: " << file_name[0] << " <input_image>" << std::endl;
            return 1;
        }

        const std::string input_filename = file_name[0];
        const std::string output_filename = file_name[1];

        // 입력 이미지 열기
        auto input = OIIO::ImageInput::open(input_filename);
        if (!input)
        {
            std::cerr << "입력 파일을 열 수 없습니다: " << input_filename << std::endl;
            ;
            return 1;
        }

        const OIIO::ImageSpec &spec = input->spec();
        const int width = spec.width;
        const int height = spec.height;
        const int channels = spec.nchannels;

        if (channels != 3)
        {
            std::cerr << "3채널(RGB) 이미지만 지원됩니다." << std::endl;
            ;
            return 1;
        }

        std::vector<float> color(width * height * channels);
        if (!input->read_image(0, 0, 0, channels, OIIO::TypeDesc::FLOAT, color.data()))
        {
            std::cerr << "이미지를 읽는 데 실패했습니다." << std::endl;
            ;
            return 1;
        }
        input->close();

        // 출력 버퍼 생성
        // std::vector<float> output(color.size());

        // OIDN 디바이스 생성 및 커밋
        oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CUDA);
        device.commit();

        // 필터 생성 및 설정
        oidn::BufferRef colorBuf = device.newBuffer(width * height * channels * sizeof(float));
        colorBuf.write(0, color.size() * sizeof(float), color.data());
        oidn::FilterRef filter = device.newFilter("RT"); // 일반적인 레이 트레이싱 필터
        filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
        filter.setImage("output", colorBuf, oidn::Format::Float3, width, height);
        filter.set("hdr", true);
        filter.commit();

        float *colorPtr = (float *)colorBuf.getData();

        // 필터 실행
        filter.execute();

        // 오류 확인
        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
        {
            std::cerr << "OIDN 오류: " << errorMessage << std::endl;
            ;
            return 1;
        }

        // 결과 이미지 저장
        auto output_file = OIIO::ImageOutput::create(output_filename);
        if (!output_file)
        {
            std::cerr << "출력 파일을 생성할 수 없습니다: " << output_filename << std::endl;
            ;
            return 1;
        }

        OIIO::ImageSpec out_spec(width, height, channels, OIIO::TypeDesc::FLOAT);
        if (!output_file->open(output_filename, out_spec))
        {
            std::cerr << "출력 파일을 여는 데 실패했습니다: " << output_filename << std::endl;
            ;
            return 1;
        }

        if (!output_file->write_image(OIIO::TypeDesc::FLOAT, colorPtr))
        {
            std::cerr << "이미지를 쓰는 데 실패했습니다." << std::endl;
            ;
            return 1;
        }
        output_file->close();

        std::cout << "노이즈 제거된 이미지를 저장했습니다: " << output_filename << std::endl;

        return 0;
    }
}
