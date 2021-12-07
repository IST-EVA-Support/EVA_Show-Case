#include "Status.h"
#include <iostream>

using namespace BASIC_INFORMATION;
using namespace PROGRESS;
using namespace PREPARE;

//===== Class BOM
BOM::BOM(std::vector<std::string> nameVector, std::vector<int> numberVector, std::vector<int> orderVector)
{
    this->NameVector = nameVector;
    this->NumberVector = numberVector;
    this->OrderVector = orderVector;
}

BOM::~BOM()
{
    
}

//===== Class PrepareStatus
PrepareStatus* PrepareStatus::singletonInstance = NULL;
PrepareStatus* PrepareStatus::GetInstance()
{
    if(singletonInstance == NULL)
        singletonInstance = new PrepareStatus();
    return singletonInstance;
}

void PrepareStatus::ReleaseInstance()
{
    if(singletonInstance != NULL)
    {
        delete singletonInstance;
        singletonInstance = NULL;
    }
}

void PrepareStatus::SetStatus(Prepare s)
{
    status = s;
}

Prepare PrepareStatus::GetStatus()
{
    return status;
}

//===== Class PartNumber
PartNumber::PartNumber(int requiredNumber)
{
    this->currentNumber = 0;
    this->requiredNumber = requiredNumber;
}

PartNumber::~PartNumber()
{
    
}

int PartNumber::GetCurrentNumber()
{
    return this->currentNumber;
}
    
void PartNumber::SetNumber(int n)
{
    this->currentNumber = n;
}

void PartNumber::ClearStatus()
{
    this->currentNumber = 0;
}

bool PartNumber::CheckNumber()
{
    return this->currentNumber == this->requiredNumber ? true : false;
}

//===== Class PositionInBox
PositionInBox::PositionInBox(int requiredIndex)
{
    this->requiredIndex = requiredIndex;
    this->currentIndex = -1;
}

PositionInBox::~PositionInBox()
{
    
}

void PositionInBox::ClearStatus()
{
    this->currentIndex = -1;
}

void PositionInBox::UpdateIndex(int currentIndex)
{
    this->currentIndex = currentIndex;
}

int PositionInBox::GetCurrentIndex()
{
    return this->currentIndex;
}

int PositionInBox::GetRequiredIndex()
{
    return this->requiredIndex;
}

bool PositionInBox::CheckPosition()
{
    return requiredIndex == currentIndex ? true : false;
}

//===== Material
Material::Material(int requiredNumber, int requiredIndex)
{
    partNumber = new PartNumber(requiredNumber);
    positionInBox = new PositionInBox(requiredIndex);
}

Material::~Material()
{
    delete partNumber;
    delete positionInBox;
}

void Material::Add(int x1, int y1, int x2, int y2, float score)
{
    std::vector<int> position = {x1, y1, x2, y2};
    positions.push_back(position);
    scoreVector.push_back(score);
}

std::vector<int> Material::GetPosition(int index)
{
    std::vector<int> pointsVector;
    if((uint)index > this->positions.size())
        return pointsVector;
    
    int x1 = this->positions[index][0];
    int y1 = this->positions[index][1];
    int x2 = this->positions[index][2];
    int y2 = this->positions[index][3];
    pointsVector = {x1, y1, x2, y2};
    return pointsVector;
}

std::vector<int> Material::GetRectangle(int index)
{
    std::vector<int> rectangle;
    if((uint)index > this->positions.size())
        return rectangle;
    
    int x1 = this->positions[index][0];
    int y1 = this->positions[index][1];
    int x2 = this->positions[index][2];
    int y2 = this->positions[index][3];
    rectangle = {x1, y1, x2-x1+1, y2-y1+1};
    return rectangle;
}

std::vector<cv::Point> Material::GetObjectPoints(int index)
{
    std::vector<cv::Point> pointsVector;
    if((uint)index > this->positions.size())
        return pointsVector;
    
    int x1 = this->positions[index][0];
    int y1 = this->positions[index][1];
    int x2 = this->positions[index][2];
    int y2 = this->positions[index][3];
    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;
    pointsVector = {cv::Point2d(x1, y1), cv::Point2d(x1 + w, y1), cv::Point2d(x2, y2), cv::Point2d(x1, y1 + h)};
    return pointsVector;
}

int Material::GetObjectNumber()
{
    return this->positions.size();
}

void Material::SetPartNumber(int n)
{
    this->partNumber->SetNumber(n);
}

void Material::SetIndexInBox(int order)
{
    this->positionInBox->UpdateIndex(order);
}

int Material::GetValidatePartCurrentNumber()
{
    return this->partNumber->GetCurrentNumber();
}

int Material::GetValidatePartCurrentIndex()
{
    return this->positionInBox->GetCurrentIndex();
}

int Material::GetPartRequiredIndex()
{
    return this->positionInBox->GetRequiredIndex();
}

int Material::GetBestScoreObjectIndex()
{
    int bestScore = -1;
    int bestObjectIndex = -1;
    for(unsigned int i = 0; i < scoreVector.size(); ++i)
    {
        if(scoreVector[i] > bestScore)
        {
            bestScore = scoreVector[i];
            bestObjectIndex = i;
        }
    }
    return bestObjectIndex;
}

void Material::ClearStatus()
{
    partNumber->ClearStatus();
    positionInBox->ClearStatus();
    for(unsigned int i = 0; i < positions.size(); ++i)
    {
        positions[i].clear();
    }
    positions.clear();
    scoreVector.clear();
}

bool Material::IsReady()
{
    return partNumber->CheckNumber() && positionInBox->CheckPosition() ? true : false;
}

bool Material::CheckNumber()
{
    return partNumber->CheckNumber() ? true : false;
}

bool Material::CheckOrder()
{
    return positionInBox->CheckPosition() ? true : false;
}
