#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "transport-catalogue/transport_catalogue.h"
#include "transport-catalogue/request_handler.h"
#include "transport-catalogue/json_reader.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_browseInputButton_clicked();
    void on_requestTypeComboBox_currentTextChanged(const QString& text);
    void on_clearButton_clicked();
    void on_runButton_clicked();

private:
    Ui::MainWindow *ui;
    tc::TransportCatalogue catalogue_;
    std::unique_ptr<tc::RequestHandler> handler_;
    std::unique_ptr<tc::JsonReader> reader_;
    renderer::RenderSettings settings_;
    bool data_loaded_ = false;
    bool has_render_settings_ = false;
    QString loaded_data_path_;
};
#endif // MAINWINDOW_H
