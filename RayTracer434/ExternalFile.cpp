#include "ExternalFile.h"


std::string ExternalFile::Load(const std::string fileName) {
    std::string fileString = "";
    std::ifstream file(fileName);
    std::string line;
    if (file.is_open()) {
        fileString = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    return fileString;
}

void ExternalFile::BMPInit(int width, int height) {

    
    m_height = height;

    uint8_t padBytes = 4 - ((width * sizeof(CV::ColorBMP)) % 4);
    if (padBytes == 4) padBytes = 0;

    m_width = width * sizeof(CV::ColorBMP) + padBytes;

    int headerSize = 54;
    
    m_fileSize = headerSize + (m_width * height);

    m_data = (unsigned char*)malloc(m_fileSize);

    //unsigned char* bmpHeader = (unsigned char*)m_data;
    BITMAPFILEHEADER* bmpHeader = (BITMAPFILEHEADER*)m_data;

    //unsigned char* bmpInfoHeader = (unsigned char*)(m_data + 14);
    BITMAPINFOHEADER* bmpInfoHeader = (BITMAPINFOHEADER*)(m_data + 14);

    m_imageBegin = (unsigned char*)m_data + headerSize;

    bmpHeader->bfType = 0x4D42;
    bmpHeader->bfSize = m_fileSize;
    bmpHeader->bfReserved1 = 0;
    bmpHeader->bfReserved2 = 0;
    bmpHeader->bfOffBits = 54;

    //code adapted from https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
    /*memset(bmpHeader, 0, 14);

    bmpHeader[0] = (unsigned char)('B');
    bmpHeader[1] = (unsigned char)('M');
    bmpHeader[2] = (unsigned char)(m_fileSize);
    bmpHeader[3] = (unsigned char)(m_fileSize >> 8);
    bmpHeader[4] = (unsigned char)(m_fileSize >> 16);
    bmpHeader[5] = (unsigned char)(m_fileSize >> 24);
    bmpHeader[10] = (unsigned char)(54);*/

    memset(bmpInfoHeader, 0, 40);


    /*bmpInfoHeader[0] = (unsigned char)(40);
    bmpInfoHeader[4] = (unsigned char)(width);
    bmpInfoHeader[5] = (unsigned char)(width >> 8);
    bmpInfoHeader[6] = (unsigned char)(width >> 16);
    bmpInfoHeader[7] = (unsigned char)(width >> 24);
    bmpInfoHeader[8] = (unsigned char)(height);
    bmpInfoHeader[9] = (unsigned char)(height >> 8);
    bmpInfoHeader[10] = (unsigned char)(height >> 16);
    bmpInfoHeader[11] = (unsigned char)(height >> 24);
    bmpInfoHeader[12] = (unsigned char)(1);
    bmpInfoHeader[14] = (unsigned char)(3 * 8);*/
    bmpInfoHeader->biSize = (unsigned char)40;
    bmpInfoHeader->biWidth = width;
    bmpInfoHeader->biHeight = height;
    bmpInfoHeader->biPlanes = (unsigned char)1;
    bmpInfoHeader->biBitCount = (unsigned char)24;
    bmpInfoHeader->biCompression = BI_RGB;

}

void ExternalFile::BMPBuffer(int x, int y, CV::ColorBMP color) {
    memcpy(m_imageBegin + (m_width * x) + y*3, &color, 3);
}

void ExternalFile::BMPWrite(std::string name) {
    FILE* pFile;
    errno_t err;
    std::string finalLoc = "Results/" + name;

    err = fopen_s(&pFile, finalLoc.c_str(), "wb");
    if (pFile) {
        size_t written = fwrite(m_data, sizeof(char), m_fileSize, pFile);

        
        fclose(pFile);
        assert(written == m_fileSize);
        free(m_data);
    }
    
}