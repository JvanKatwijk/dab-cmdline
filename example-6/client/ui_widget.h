/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 5.12.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>

QT_BEGIN_NAMESPACE

class Ui_widget
{
public:
    QPushButton *connectButton;
    QLineEdit *hostLineEdit;
    QLabel *state;
    QPushButton *terminateButton;
    QListView *ensembleDisplay;
    QLabel *ensembleLabel;
    QLabel *dynamicLabel;
    QComboBox *channelSelector;
    QSlider *gainSlider;
    QListView *programView;

    void setupUi(QDialog *widget)
    {
        if (widget->objectName().isEmpty())
            widget->setObjectName(QString::fromUtf8("widget"));
        widget->resize(499, 312);
        connectButton = new QPushButton(widget);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));
        connectButton->setGeometry(QRect(10, 10, 131, 21));
        hostLineEdit = new QLineEdit(widget);
        hostLineEdit->setObjectName(QString::fromUtf8("hostLineEdit"));
        hostLineEdit->setGeometry(QRect(10, 30, 131, 21));
        state = new QLabel(widget);
        state->setObjectName(QString::fromUtf8("state"));
        state->setGeometry(QRect(320, 80, 131, 31));
        terminateButton = new QPushButton(widget);
        terminateButton->setObjectName(QString::fromUtf8("terminateButton"));
        terminateButton->setGeometry(QRect(140, 10, 71, 41));
        ensembleDisplay = new QListView(widget);
        ensembleDisplay->setObjectName(QString::fromUtf8("ensembleDisplay"));
        ensembleDisplay->setGeometry(QRect(255, 130, 221, 171));
        ensembleLabel = new QLabel(widget);
        ensembleLabel->setObjectName(QString::fromUtf8("ensembleLabel"));
        ensembleLabel->setGeometry(QRect(10, 100, 201, 16));
        dynamicLabel = new QLabel(widget);
        dynamicLabel->setObjectName(QString::fromUtf8("dynamicLabel"));
        dynamicLabel->setGeometry(QRect(10, 70, 241, 16));
        channelSelector = new QComboBox(widget);
        channelSelector->setObjectName(QString::fromUtf8("channelSelector"));
        channelSelector->setGeometry(QRect(210, 10, 98, 41));
        gainSlider = new QSlider(widget);
        gainSlider->setObjectName(QString::fromUtf8("gainSlider"));
        gainSlider->setGeometry(QRect(0, 140, 24, 160));
        gainSlider->setMaximum(100);
        gainSlider->setValue(50);
        gainSlider->setOrientation(Qt::Vertical);
        programView = new QListView(widget);
        programView->setObjectName(QString::fromUtf8("programView"));
        programView->setGeometry(QRect(30, 131, 211, 171));

        retranslateUi(widget);

        QMetaObject::connectSlotsByName(widget);
    } // setupUi

    void retranslateUi(QDialog *widget)
    {
        widget->setWindowTitle(QApplication::translate("widget", "Dialog", nullptr));
        connectButton->setText(QApplication::translate("widget", "connect", nullptr));
        state->setText(QString());
        terminateButton->setText(QApplication::translate("widget", "quit", nullptr));
        ensembleLabel->setText(QString());
        dynamicLabel->setText(QApplication::translate("widget", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class widget: public Ui_widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
