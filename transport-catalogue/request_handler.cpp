#include "request_handler.h"

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& catalogue,
                               const renderer::MapRenderer& renderer)
    : catalogue_(catalogue), renderer_(renderer) {
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.Render(catalogue_.GetAllBuses());
}
