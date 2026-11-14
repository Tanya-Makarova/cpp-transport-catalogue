#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"

class RequestHandler {
public:
    RequestHandler(const transport_catalogue::TransportCatalogue& catalogue,
                   const renderer::MapRenderer& renderer);

    svg::Document RenderMap() const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    const renderer::MapRenderer& renderer_;
};
