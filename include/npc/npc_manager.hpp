#pragma once
#include "npc.hpp"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class NPCManager {
public:

    void addNPC(const std::shared_ptr<NPC>& npc) {
        npcs.push_back(npc);
    }

    std::shared_ptr<NPC> getNearestNPC(const glm::vec3& playerPos, float maxDistance) const {
        std::shared_ptr<NPC> nearest = nullptr;
        float minDist = maxDistance;
        for (const auto& npc : npcs) {
            float dist = glm::distance(npc->getPosition(), playerPos);
            if (dist < minDist) {
                minDist = dist;
                nearest = npc;
            }
        }
        return nearest;
    }

    void updateAll() {
        // Aggiorna lo stato di tutti gli NPC, se necessario
    }

    const std::vector<std::shared_ptr<NPC>>& getNPCs() const {
        return npcs;
    }

    int init(std::string file, AssetFile** af) {
        nlohmann::json sceneJson;
        std::ifstream ifs(file);
        if (!ifs.is_open()) {
            std::cout << "Error! NPC file >" << file << "< not found!";
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
        if (!sceneJson.contains("npcs") || !sceneJson["npcs"].is_array()) return -1;

        int skinId = 0; // Default skin ID, can be modified if needed
        for (const auto& npcJson : sceneJson["npcs"]) {
            std::string name = npcJson.value("name", "Unknown");
            auto posArr = npcJson.value("position", std::vector<float>{0,0,0});
            glm::vec3 pos = glm::vec3(posArr[0], posArr[1], posArr[2]);
            auto animList = npcJson.value("animList", std::vector<std::string>{});
            auto npcStates = npcJson.value("npcStates", std::vector<std::string>{});
            auto startEndFrames = npcJson.value("startEndFrames", std::vector<std::vector<int>>{});
            std::string baseTrack = npcJson.value("BaseTrackName", "");

            // Inizializza Animations
            int animCount = static_cast<int>(animList.size());
            // Animations Anim[animCount];
            // std::vector<Animations> Anim(animCount);
            std::cout << "DAWdadwad \n";
            // Inizializza Anims[skinId] come un vettore di Animations di dimensione animCount
            if (Anims.size() <= skinId) {
                Anims.resize(skinId + 1);
            }
            Anims[skinId].resize(animCount);
            for (int ian = 0; ian < animCount; ian++) {
                //Anim[ian].init(*af[ian]);
                // std::cout << "ALAKAZAM \n";
                Anims[skinId][ian].init(*af[ian]);
                // std::cout << "NOPERS \n";
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
            SKA->init(Anims[skinId].data(), animCount, baseTrack, skinId);
            skinId++;

            // Crea NPC e aggiungi
            auto npc = std::make_shared<NPC>(name, pos, ab, SKA, npcStates);
            addNPC(npc);
        }
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
        // npcs.clear();
    }



private:
    std::vector<std::shared_ptr<NPC>> npcs;
    std::vector<std::vector<Animations>> Anims; // Per cleanuppare le animazioni degli NPC
};