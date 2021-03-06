#include "imageanticolordlg.h"
#include "ui_imageanticolordlg.h"
#include "QMessageBox"
#include "QFileDialog"

ImageAnticolorDlg::ImageAnticolorDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageAnticolorDlg)
{
    ui->setupUi(this);
    QObject::connect(ui->pushButtonInput, &QPushButton::clicked, this, &ImageAnticolorDlg::slotsInput);
    QObject::connect(ui->pushButtonOutput, &QPushButton::clicked, this, &ImageAnticolorDlg::slotsOutput);
    QObject::connect(ui->pushButtonApply, &QPushButton::clicked, this, &ImageAnticolorDlg::slotsApply);

}

ImageAnticolorDlg::~ImageAnticolorDlg()
{
    delete ui;
}

void ImageAnticolorDlg::slotsInput()
{
    QString strSrcFilePath = QFileDialog::getOpenFileName(this,
                                                          tr("打开原始图像"),
                                                          ".",
                                                          "Image fils(*.tif);;All files(*.*)");
    if(strSrcFilePath == NULL)
    {
        QMessageBox::information(this, tr("Tips"), "Cant Open the File");
        return ;
    }
    ui->lineEditInput->setText(strSrcFilePath);

}
void ImageAnticolorDlg::slotsOutput()
{
    QString strDstFilePath = QFileDialog::getSaveFileName(this,
                                                          tr("保存"),
                                                          ".",
                                                          "Image files(*.tif)");
    if(strDstFilePath == NULL)
    {
        QMessageBox::information(this, tr("Tips"), "Select the Save File");
        return ;
    }
    ui->lineEditOutput->setText(strDstFilePath);
}
void ImageAnticolorDlg::slotsApply()
{
    QString pszSrcFile;
    pszSrcFile = ui->lineEditInput->text();
    QString pszDstFile;
    pszDstFile = ui->lineEditOutput->text();

    ///矢量化实例
    CProgressDlg* pProcess = new CProgressDlg();
    pProcess->setWindowTitle(tr("Polygonize"));
    pProcess->show();

    ImageAnticolor(pszSrcFile.toStdString().c_str(), pszDstFile.toStdString().c_str(), "GTiff", pProcess);

    delete pProcess;
}

int ImageAnticolorDlg::ImageAnticolor(const char *pszSrcFile, const char *pszDstFile,
                                       const char *pszFormat, CProgressBase *pProcess)
{
    if(pProcess != NULL)
    {
        pProcess->ReSetProcess();
        pProcess->SetProgressTip("Running...");
    }

    GDALAllRegister();
    GDALDataset* poSrcDS = (GDALDataset* )GDALOpen(pszSrcFile, GA_ReadOnly);
    if(poSrcDS == NULL)
    {
        if(pProcess != NULL)
            pProcess->SetProgressTip("can not open the file");
        return 0;
    }

    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if(poDriver == NULL)
    {
        if(pProcess != NULL)
            pProcess->SetProgressTip("can not create the type");
        GDALClose((GDALDatasetH)poSrcDS);
        return 0;
    }
    //获取图像的宽高和波段数
    int iXSize = poSrcDS->GetRasterXSize();
    int iYSize = poSrcDS->GetRasterYSize();
    int iBandCount = poSrcDS->GetRasterCount();

    GDALDataset* poDstDS = poDriver->Create(pszDstFile, iXSize, iYSize, iBandCount, GDT_Byte, NULL);
    double dGeoTrans[6] = {0};
    //设置仿射变换参数
    poSrcDS->GetGeoTransform(dGeoTrans);
    poDstDS->SetGeoTransform(dGeoTrans);
    //设置投影信息
    poDstDS->SetProjection(poSrcDS->GetProjectionRef());

    unsigned char* pSrcData = new unsigned char[iXSize];
    unsigned char* pDstData = new unsigned char[iXSize];

    if(pProcess != NULL)
    {
        pProcess->SetProgressTip("cal image anticolor...");
        pProcess->SetStepCount(iYSize * iBandCount);//设置进度条的总长度
    }
    for(int iBand = 1;iBand <= iBandCount; iBand++)//循环波段
    {
        GDALRasterBand* poSrcBand = poSrcDS->GetRasterBand(iBand);
        GDALRasterBand* poDstBand = poDstDS->GetRasterBand(iBand);
        for(int i = 0; i < iYSize; i++)//循环图像的高
        {
            //一行一行的读取数据
            poSrcBand->RasterIO(GF_Read, 0, i, iXSize, 1, pSrcData, iXSize, 1, GDT_Byte, 0, 0);
            for(int j = 0; j < iXSize; j++ ) //循环每一行
            {
                pDstData[j] = 255 - pSrcData[j];
            }
            poDstBand->RasterIO(GF_Write, 0, i, iXSize, 1, pDstData, iXSize, 1, GDT_Byte, 0, 0);

            if(pProcess != NULL)
            {
                bool IsCancle =pProcess->StepIt();
                if(!IsCancle)
                {
                    delete pSrcData;
                    delete pDstData;
                    GDALClose((GDALDatasetH)poSrcDS);
                    GDALClose((GDALDatasetH)poDstDS);
                    return 0;
                }
            }
        }
    }

    delete pSrcData;
    delete pDstData;
    GDALClose((GDALDatasetH)poSrcDS);
    GDALClose((GDALDatasetH)poDstDS);

    if(pProcess != NULL)
    {
        pProcess->SetProgressTip("Finished!");
    }


    return 1;
}
