#include "uti.h"

namespace uti {
    map<int, map<int, string>> categories = {
        {
            Language::FR,   {
                                { Category::PLAYER, "Personnage joueur"    },
                                { Category::NPC   , "Personnage non joueur"}
                            },
        },
        {
            Language::ENG,  {
                                { Category::PLAYER, "Player character"       },
                                { Category::NPC   , "Non-playable character" }
                            }
        }
    };

    std::map<float, MoveRate> pixDir = {
    {0,   {0,  -1}},
    {1,   {1,   0}},
    {2,   {0,   1}},
    {3,  {-1,  0}},
    {0.5,   {0.5, -0.5}},
    {3.5,  {-0.5,-0.5}},
    {1.5,   {0.5,  0.5}},
    {2.5,  {-0.5, 0.5}},
    };
    uint64_t getCurrentTimestamp()
    {
        return static_cast<uint64_t>(std::time(nullptr));
    }

    Uint32 get_pixel(SDL_Surface* surface, int x, int y)
    {
        // Obtenir le format de pixel de la surface
        SDL_PixelFormat* format = surface->format;

        // Calculer l'offset du pixel
        int bpp = format->BytesPerPixel;
        Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

        // Lire la valeur du pixel selon le format de pixel
        switch (bpp) {
        case 1:
            return *p;
        case 2:
            return *(Uint16*)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
            return *(Uint32*)p;
        default:
            return 0; // Format non supporté
        }
    }
    bool isPointInCircle(const short x, const short y, const short circleCenterX, const short circleCenterY, const short circleRadius)
    {
        {
            float dx = x - circleCenterX;
            float dy = y - circleCenterY;
            float distance = std::sqrt(dx * dx + dy * dy);
            return distance <= circleRadius;
        }
    }
    bool doCirclesIntersect(const Circle& c1, const Circle& c2)
    {
        float dx = c2.center.x - c1.center.x;
        float dy = c2.center.y - c1.center.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Vérifier si les cercles se croisent
        return (distance <= (c1.radius + c2.radius)) && (distance >= std::abs(c1.radius - c2.radius));
    }
}