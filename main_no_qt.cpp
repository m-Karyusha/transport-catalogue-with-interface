// #include <iostream>

// #include "json.h"
// #include "json_reader.h"
// #include "map_renderer.h"
// #include "request_handler.h"
// #include "transport_catalogue.h"

// int main() {
//     tc::TransportCatalogue catalogue;
//     tc::RequestHandler handler(catalogue);
//     tc::JsonReader j_reader(catalogue);

//     json::Document input_doc = json::Load(std::cin);

//     j_reader.LoadData(input_doc);

//     // Создание json-файла статистики
//     //json::Document output_doc = j_reader.ProcessRequests(input_doc, handler);
//     //j_reader.PrintDoc(output_doc, std::cout);

//     // Создание json-файла со статистикой и картой
//     std::string rendered_map;
//     if (j_reader.HasMapRequests(input_doc)) {
//         renderer::RenderSettings settings = j_reader.LoadRenderSettings(input_doc);
//         renderer::MapRenderer map_renderer(settings);
//         rendered_map = handler.RenderMap(map_renderer);
//     }
//     json::Document output_doc = j_reader.ProcessRequests(input_doc, handler, rendered_map);
//     j_reader.PrintDoc(output_doc, std::cout);

//     // Создание svg-файла
//     //svg::Document doc = map_renderer.Render(handler.GetBusesForRender());
//     //doc.Render(std::cout);
//     return 0;
// }
