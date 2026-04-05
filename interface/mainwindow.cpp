#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QString>

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "libs/json/json.h"
#include "transport-catalogue/map_renderer.h"

namespace {

json::Document BuildRequestsDocument(const QString& request_type, const QString& request_name) {
    json::Array stat_requests;
    json::Dict request;

    request["id"] = 1;
    request["type"] = request_type.toStdString();

    if (request_type == "Stop" || request_type == "Bus") {
        request["name"] = request_name.toStdString();
    }

    stat_requests.emplace_back(std::move(request));

    json::Dict root;
    root["stat_requests"] = std::move(stat_requests);

    return json::Document(std::move(root));
}

void SaveTextToFile(const QString& file_path, const std::string& text) {
    std::ofstream out(file_path.toStdString(), std::ios::binary);
    if (!out) {
        throw std::runtime_error("Не удалось открыть выходной файл");
    }
    out << text;
}

std::string DocumentToString(const json::Document& doc) {
    std::ostringstream out;
    json::Print(doc, out);
    return out.str();
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , catalogue_()
    , handler_(std::make_unique<tc::RequestHandler>(catalogue_))
    , reader_(std::make_unique<tc::JsonReader>(catalogue_)) {
    ui->setupUi(this);

    ui->requestNameLineEdit->setEnabled(true);
    statusBar()->showMessage("Готово");
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_browseInputButton_clicked() {
    const QString file_name = QFileDialog::getOpenFileName(
        this,
        "Выберите файл с данными",
        QString(),
        "JSON files (*.json);;All files (*.*)"
        );

    if (file_name.isEmpty()) {
        return;
    }

    try {
        std::ifstream input(file_name.toStdString(), std::ios::binary);
        if (!input) {
            QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл с данными.");
            return;
        }

        json::Document base_doc = json::Load(input);

        // Дозагрузка в существующий каталог
        catalogue_.Clear();
        reader_->LoadData(base_doc);

        const auto& root = base_doc.GetRoot().AsMap();
        if (root.count("render_settings")) {
            settings_ = tc::JsonReader::LoadRenderSettings(base_doc);
            has_render_settings_ = true;
        } else {
            has_render_settings_ = false;
        }

        ui->inputFileLineEdit->setText(file_name);
        data_loaded_ = true;

        statusBar()->showMessage("Данные успешно загружены", 3000);
        QMessageBox::information(this, "Готово", "Данные добавлены в каталог.");
    } catch (const json::ParsingError& e) {
        QMessageBox::critical(
            this,
            "Ошибка JSON",
            QString("Ошибка разбора JSON:\n%1").arg(e.what())
            );
    } catch (const std::exception& e) {
        QMessageBox::critical(
            this,
            "Ошибка",
            QString("Во время загрузки произошла ошибка:\n%1").arg(e.what())
            );
    }
}

void MainWindow::on_requestTypeComboBox_currentTextChanged(const QString& text) {
    const bool is_map = (text == "Map");

    ui->requestNameLineEdit->setEnabled(!is_map);

    if (is_map) {
        ui->requestNameLineEdit->clear();
        ui->requestNameLineEdit->clear();
        ui->outputFileLineEdit->setText("map.svg");
        ui->requestNameLineEdit->setPlaceholderText("Для карты название не требуется");
    } else if (text == "Stop") {
        ui->outputFileLineEdit->setText("result.json");
        ui->requestNameLineEdit->setPlaceholderText("Введите название остановки");
    } else if (text == "Bus") {
        ui->outputFileLineEdit->setText("result.json");
        ui->requestNameLineEdit->setPlaceholderText("Введите название маршрута");
    } else {
        ui->requestNameLineEdit->setPlaceholderText("Введите название");
    }
}

void MainWindow::on_clearButton_clicked() {
    catalogue_.Clear();

    handler_ = std::make_unique<tc::RequestHandler>(catalogue_);
    reader_ = std::make_unique<tc::JsonReader>(catalogue_);

    has_render_settings_ = false;
    data_loaded_ = false;
    loaded_data_path_.clear();

    ui->inputFileLineEdit->clear();
    ui->outputFileLineEdit->clear();
    ui->requestNameLineEdit->clear();

    statusBar()->showMessage("База очищена", 3000);
}

void MainWindow::on_runButton_clicked() {
    try {
        const QString input_path = ui->inputFileLineEdit->text().trimmed();
        const QString output_name = ui->outputFileLineEdit->text().trimmed();
        const QString request_type = ui->requestTypeComboBox->currentText();
        const QString request_name = ui->requestNameLineEdit->text().trimmed();

        if (input_path.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Не выбран файл с данными.");
            return;
        }

        if (output_name.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Введите имя выходного файла.");
            return;
        }

        if ((request_type == "Stop" || request_type == "Bus") && request_name.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Введите название остановки или маршрута.");
            return;
        }

        const json::Document requests_doc = BuildRequestsDocument(request_type, request_name);
        const QString output_path = QDir::current().filePath(output_name);

        if (request_type == "Map") {
            if (!has_render_settings_) {
                QMessageBox::warning(this, "Ошибка", "В загруженных данных нет render_settings для построения карты.");
                return;
            }

            renderer::MapRenderer map_renderer(settings_);
            std::string svg = handler_->RenderMap(map_renderer);
            SaveTextToFile(output_path, svg);
        } else {
            json::Document result_doc = tc::JsonReader::ProcessRequests(requests_doc, *handler_);
            SaveTextToFile(output_path, DocumentToString(result_doc));
        }

        statusBar()->showMessage("Запрос выполнен успешно", 3000);
        QMessageBox::information(
            this,
            "Готово",
            QString("Результат сохранён в файл:\n%1").arg(output_path)
            );
    } catch (const json::ParsingError& e) {
        QMessageBox::critical(
            this,
            "Ошибка JSON",
            QString("Ошибка разбора JSON:\n%1").arg(e.what())
            );
    } catch (const std::exception& e) {
        QMessageBox::critical(
            this,
            "Ошибка",
            QString("Во время выполнения произошла ошибка:\n%1").arg(e.what())
            );
    }
}
