#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QWidget>

namespace Ui {
class Configuration;
}

class Configuration : public QWidget
{
    Q_OBJECT

public:
    explicit Configuration(QWidget *parent = nullptr);
    ~Configuration();



private slots:
    void on_cbCustomer_currentIndexChanged(int index);
    void on_cbHighCurrent_currentIndexChanged(int index);

    void on_btnSubmit_clicked();

    void on_cbSetSerial_currentIndexChanged(int index);

signals:
    emit void submitClicked();
private:
    void refresh();
    void fillValues();
    bool validate();

private:
    Ui::Configuration *ui;
};

#endif // CONFIGURATION_H
