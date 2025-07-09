#pragma once
#include "character.hpp"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class CharManager {
public:

    void addChar(const std::shared_ptr<Character>& character) {
        characters.push_back(character);
    }

    // Restituisce il Character interactable più vicino alla posizione del giocatore, entro una distanza massima
    std::shared_ptr<Character> getNearestCharacter(const glm::vec3& playerPos) const {
        std::shared_ptr<Character> nearest = nullptr;
        float minDist = maxDistance;
        for (const auto& charac : characters) {
            float dist = glm::distance(charac->getPosition(), playerPos);
            if (dist < minDist && charac->getState() == "Idle") {
                minDist = dist;
                nearest = charac;
            }
        }
        return nearest;
    }

    void updateAll() {
        // Aggiorna lo stato di tutti gli Character, se necessario
    }

    const std::vector<std::shared_ptr<Character>>& getCharacters() const {
        return characters;
    }

    int init(std::string file, AssetFile** af) {
        nlohmann::json sceneJson;
        std::ifstream ifs(file);
        if (!ifs.is_open()) {
            std::cout << "Error! Character file >" << file << "< not found!";
            exit(-1);
        }
        try {
            ifs >> sceneJson;
            ifs.close();
        } catch (const nlohmann::json::exception& e) {
            std::cout << "\n\n\nException while parsing JSON file: " << file << "\n";
            std::cout << e.what() << '\n' << '\n';
            std::cout << std::flush;
            return 1;
        }
        if (!sceneJson.contains("characters") || !sceneJson["characters"].is_array()) return -1;

        int skinId = 0; // Default skin ID, can be modified if needed

        // Crea una mappa stringa (id asset file) -> indice nell'array degli AssetFile
        std::unordered_map<std::string, int> assetFileIdToIndex;
        int assetFileIdx = 0;
        for (const auto& assetFile : sceneJson["assetfiles"]) {
            if (assetFile.contains("id")) {
                assetFileIdToIndex.insert({assetFile["id"].get<std::string>(), assetFileIdx});
            }
            assetFileIdx++;
        }

        for (const auto& charJson : sceneJson["characters"]) {
            std::string name = charJson.value("name", "Unknown");
            auto posArr = charJson.value("position", std::vector<float>{0,0,0});
            // TODO: get position from the scene file (cook-torranceChar instances)
            glm::vec3 pos = glm::vec3(posArr[0], posArr[1], posArr[2]);
            auto animList = charJson.value("animList", std::vector<std::string>{});
            auto assetFilesList = sceneJson["assetfiles"];
            auto charStates = charJson.value("charStates", std::vector<std::string>{});
            auto startEndFrames = charJson.value("startEndFrames", std::vector<std::vector<int>>{});
            std::string baseTrack = charJson.value("BaseTrackName", "");

            // Inizializza Animations
            int animCount = static_cast<int>(animList.size());
            // Inizializza Anims[skinId] come un vettore di Animations di dimensione animCount
            if (Anims.size() <= skinId) {
                Anims.resize(skinId + 1);
            }
            Anims[skinId].resize(animCount);
            for (int ian = 0; ian < animCount; ian++) {

                // Trova l'AssetFile corrispondente all'ID dell'animazione
                const std::string& idToSeek = animList[ian];
                if (assetFileIdToIndex.find(idToSeek) != assetFileIdToIndex.end()) {
                    assetFileIdx = assetFileIdToIndex[idToSeek];
                } else {
                    std::cout << "Error! Animation ID >" << idToSeek << "< not found in asset files list.\n";
                    return -1;
                }

                Anims[skinId][ian].init(*af[assetFileIdx]);
                std::cout << "ANIM " << ian << " of char " << skinId << " initialized with asset file: " << assetFilesList[assetFileIdx]["id"] << "\n";
            }

            // Inizializza AnimBlender
            std::vector<AnimBlendSegment> segments(startEndFrames.size());
            for (size_t i = 0; i < startEndFrames.size(); ++i) {
                segments[i] = {startEndFrames[i][0], startEndFrames[i][1], 0.0f, static_cast<int>(i)};
                // std::cout << "Starting frame: " << startEndFrames[i][0] << " Ending frame: " << startEndFrames[i][1] <<"\n";
            }
            auto ab = std::make_shared<AnimBlender>();
            ab->init(segments);
            // std::cout << "First segment starts at " << ab->segments[0].st << " and ends at " << ab->segments[0].en << "\n";


            // Inizializza SkeletalAnimation
            auto SKA = std::make_shared<SkeletalAnimation>();
            SKA->init(Anims[skinId].data(), animCount, baseTrack);
            skinId++;

            // Crea Character e aggiungi
            auto charac = std::make_shared<Character>(name, pos, ab, SKA, charStates);
            addChar(charac);
        }
        std::cout << "Characters initialization finished \n";
        return 0;
    }

    std::vector<Animations>& getAnims() {
         return Anims[0]; // Ritorna l'ultima animazione caricata
    }

    void cleanup() {
        for (int i = 0; i < Anims.size(); ++i) {
            for (int j = 0; j < Anims[i].size(); ++j) {
                Anims[i][j].cleanup();
            }
            // Anims[i].cleanup();
        }
        // if (Anim) {
        //     delete[] Anim;
        //     Anim = nullptr;
        // }
        // characters.clear();
    }



private:
    std::vector<std::shared_ptr<Character>> characters;
    std::vector<std::vector<Animations>> Anims; // Per cleanuppare le animazioni degli Character
    float maxDistance = 7.5f; // Distanza massima per considerare un Character "vicino"
};