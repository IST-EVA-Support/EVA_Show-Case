#ifndef __DEMO_STATUS_H__
#define __DEMO_STATUS_H__

#include <vector>
#include <opencv2/opencv.hpp>

namespace BASIC_INFORMATION
{
    class BOM
    {
    public:
        BOM(std::vector<std::string> nameVector, std::vector<int> NumberVector, std::vector<int> OrderVector);
        ~BOM();
    public:
        std::vector<std::string> NameVector;
        std::vector<int> NumberVector;
        std::vector<int> OrderVector;
    };
}

namespace PROGRESS
{
    enum class Prepare {Empty, NotReady, DisOrdered, Ready, Assembly};
    enum class Assembly {Empty, Assembling, AssemblyOK};
    
    class PrepareStatus
    {
    public:
        static PrepareStatus* GetInstance();
        void ReleaseInstance();
        void SetStatus(Prepare s);
        Prepare GetStatus();
        const char* GetStatusString(Prepare value)
        {
            switch (value) {
                case Prepare::Empty:
                    return "Empty";
                case Prepare::NotReady:
                    return "NotReady";
                case Prepare::DisOrdered:
                    return "DisOrdered";
                case Prepare::Ready:
                    return "Ready";
                case Prepare::Assembly:
                    return "Assembly";
            }
            return "Not Defined";
        }
        
    protected:
        Prepare status;
        
    private:
        static PrepareStatus* singletonInstance;
        PrepareStatus(): status(Prepare::NotReady){};
        PrepareStatus(const PrepareStatus&);
        PrepareStatus& operator=(const PrepareStatus&);
    };
}

namespace PREPARE
{
class PartNumber
{
public:
    PartNumber(int requiredNumber);
    ~PartNumber();
    int GetCurrentNumber();
    void SetNumber(int n);
    void ClearStatus();
    bool CheckNumber();
    
private:
    int currentNumber;
    int requiredNumber;
};

class PositionInBox
{
public:
    PositionInBox(int requiredIndex);
    ~PositionInBox();
    void ClearStatus();
    void UpdateIndex(int currentIndex);
    int GetCurrentIndex();
    int GetRequiredIndex();
    bool CheckPosition();
    
private:
    int requiredIndex;
    int currentIndex;
};

class Material
{
public:
    Material(int requiredNumber, int requiredIndex);
    ~Material();
    void Add(int x1, int y1, int x2, int y2, float score);
    std::vector<int> GetPosition(int index);
    std::vector<int> GetRectangle(int index);
    std::vector<cv::Point> GetObjectPoints(int index);
    int GetObjectNumber();
    void SetPartNumber(int n);
    void SetIndexInBox(int order);
    int GetValidatePartCurrentNumber();
    int GetValidatePartCurrentIndex();
    int GetPartRequiredIndex();
    int GetBestScoreObjectIndex();
    void ClearStatus();
    bool IsReady();
    bool CheckNumber();
    bool CheckOrder();
    
private:
    PartNumber *partNumber;
    PositionInBox *positionInBox;
    std::vector<std::vector<int>> positions;
    std::vector<float> scoreVector;
};
}
#endif /* __DEMO_STATUS_H__ */
