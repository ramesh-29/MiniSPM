#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

//    benchMeterTest = new BenchMeterTest();
    spmBenchTest = new SPMBenchFullTest();


}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_Start_Test_clicked()
{
//    benchMeterTest->startTest();
    spmBenchTest->setupPowerSource();

}
