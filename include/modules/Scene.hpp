
struct TechniqueInstances;

struct Instance {
	std::string *id;
	int Mid;
	int NTx;
	int Iid;
	int *Tid;
	DescriptorSet ***DS;
	std::vector<DescriptorSetLayout *> **D;
	int *NDs;
	
	glm::mat4 Wm;
	TechniqueInstances *TIp;
} ;

struct TextureDefs {
	bool fromInstance;
	int pos;
	VkDescriptorImageInfo info;
} ;

struct PipelineAndTexturesDefs {
	Pipeline *P;
	std::vector<std::vector<TextureDefs>> texDefs;
} ;

struct TechniqueRef {
	std::string *id;
	std::vector<PipelineAndTexturesDefs>PT;
	int Ntextures;
	VertexDescriptor *VD;

	void init(const char *_id, std::vector<PipelineAndTexturesDefs> _PT, int _Ntextures, VertexDescriptor * _VD);
} ;

struct VertexDescriptorRef {
	std::string *id;
	VertexDescriptor *VD;
	
	void init(const char *_id, VertexDescriptor * _VD);
} ;

struct TechniqueInstances {
	Instance *I;
	int InstanceCount;
	
	TechniqueRef *T;
} ;


class Scene {
	public:
	
	BaseProject *BP;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	// Asset files
	int AssetFileCount = 0;
	AssetFile **As;
	std::unordered_map<std::string, int> AsIds;

	// Models
	int ModelCount = 0;
	Model **M;
	std::unordered_map<std::string, int> MeshIds;

	// Textures
	int TextureCount = 0;
	Texture **T;
	std::unordered_map<std::string, int> TextureIds;
	
	// Descriptor sets and instances
	int InstanceCount = 0;

	Instance **I;
	VertexDescriptorRef *VRef;
	std::unordered_map<std::string, int> InstanceIds;

	// Pipelines, DSL and Vertex Formats
	std::unordered_map<std::string, TechniqueRef *> TechniqueIds;
	int TechniqueInstanceCount = 0;
	TechniqueInstances *TI;
	std::unordered_map<std::string, VertexDescriptor *> VDIds;
	int Npasses;


	int init(BaseProject *_BP,  int _Npasses, std::vector<VertexDescriptorRef>  &VDRs, std::vector<TechniqueRef> &PRs, std::string file);

	void pipelinesAndDescriptorSetsInit();
	void pipelinesAndDescriptorSetsCleanup();
	void localCleanup();
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int passId, int currentImage);
};

#ifdef SCENE_IMPLEMENTATION

void TechniqueRef::init(const char *_id, std::vector<PipelineAndTexturesDefs> _PT, int _Ntextures, VertexDescriptor * _VD) {
	id = new std::string(_id);
	PT = _PT;
	Ntextures = _Ntextures;
	VD = _VD;
}

void VertexDescriptorRef::init(const char *_id, VertexDescriptor * _VD) {
	id = new std::string(_id);
	VD = _VD;
}

int Scene::init(BaseProject *_BP,  int _Npasses, std::vector<VertexDescriptorRef>  &VDRs,  
		  std::vector<TechniqueRef> &PRs, std::string file) {
	BP = _BP;
	Npasses = _Npasses;
	
	for(int i = 0; i < VDRs.size(); i++) {
		VDIds[*VDRs[i].id] = VDRs[i].VD;
	}
	for(int i = 0; i < PRs.size(); i++) {
		if(PRs[i].PT.size() != Npasses) {
			std::cout << "Scene Error: the number of pipelines for technique " << *PRs[i].id << " does not correspond to the number of passes : " << PRs[i].PT.size() << " != " << Npasses << "\n";
			exit(0);
		}
		TechniqueIds[*PRs[i].id] = &PRs[i];
	}

	// Models, textures and Descriptors (values assigned to the uniforms)
	nlohmann::json js;
	std::ifstream ifs(file);
	if (!ifs.is_open()) {
	  std::cout << "Error! Scene file >" << file << "< not found!";
	  exit(-1);
	}
//		try {
		std::cout << "Parsing JSON\n";
		ifs >> js;
		ifs.close();
		std::cout << "\nScene contains " << js.size() << " definitions sections\n\n";
		
		// ASSET FILES
		nlohmann::json afs = js["assetfiles"];
		AssetFileCount = afs.size();
		std::cout << "Asset Files count: " << AssetFileCount << "\n";

		As = (AssetFile **)calloc(AssetFileCount, sizeof(AssetFile *));
		for(int k = 0; k < AssetFileCount; k++) {
			AsIds[afs[k]["id"]] = k;
			std::string MT = afs[k]["format"].template get<std::string>();

			As[k] = new AssetFile();
			As[k]->init(afs[k]["file"], (MT[0] == 'O') ? OBJ : ((MT[0] == 'G') ? GLTF : MGCG));
			if (MT[0] == 'G') {
				// Solo se è un GLTF
				tinygltf::TinyGLTF loader;
				tinygltf::Model model;
				std::string warn, err;
				std::string path = afs[k]["file"];

				bool ok = loader.LoadASCIIFromFile(&model, &warn, &err, path);
				if (!ok) {
					std::cerr << "Failed to load GLTF file for debug: " << path << "\n";
					std::cerr << "Error: " << err << "\n";
				} else {
					std::cout << "\n=== DEBUG INFO FROM: " << path << " ===\n";
					for (size_t m = 0; m < model.meshes.size(); ++m) {
						const auto& mesh = model.meshes[m];
						std::cout << "Mesh " << m << ": " << mesh.name << "\n";
						for (size_t p = 0; p < mesh.primitives.size(); ++p) {
							const auto& prim = mesh.primitives[p];
							std::cout << "  Primitive " << p << ":\n";
							for (const auto& attr : prim.attributes) {
								std::cout << "    Attribute: " << attr.first << "\n";
							}
						}
					}
					std::cout << "Skins: " << model.skins.size() << "\n";
					std::cout << "Animations: " << model.animations.size() << "\n";
					std::cout << "===============================\n";
				}
			}

		}
		
		// MODELS
		nlohmann::json ms = js["models"];
		ModelCount = ms.size();
		std::cout << "Models count: " << ModelCount << "\n";

		M = (Model **)calloc(ModelCount, sizeof(Model *));
		for(int k = 0; k < ModelCount; k++) {
			MeshIds[ms[k]["id"]] = k;
			std::string MT = ms[k]["format"].template get<std::string>();
			std::string VDN = ms[k]["VD"].template get<std::string>();

			M[k] = new Model();
			if(MT[0] == 'A') {
				// init from asset file
				std::string AN = ms[k]["asset"].template get<std::string>();
//std::cout << "Getting from asset: '" << AN << "'\n";
				int aId = AsIds[AN];
//std::cout << "aId " << aId << "\n";
				M[k]->initFromAsset(BP, VDIds[VDN], As[aId], ms[k]["model"], ms[k]["meshId"], ms[k]["node"]);
			} else {
				M[k]->init(BP, VDIds[VDN], ms[k]["model"], (MT[0] == 'O') ? OBJ : ((MT[0] == 'G') ? GLTF : MGCG));
			}
		}
		
		// TEXTURES
		nlohmann::json ts = js["textures"];
		TextureCount = ts.size();
		std::cout << "Textures count: " << TextureCount << "\n";

		T = (Texture **)calloc(TextureCount, sizeof(Texture *));
		for(int k = 0; k < TextureCount; k++) {
			TextureIds[ts[k]["id"]] = k;
			std::string TT = ts[k]["format"].template get<std::string>();

			T[k] = new Texture();
			if(TT[0] == 'C') {
				T[k]->init(BP, ts[k]["texture"]);
			} else if(TT[0] == 'D') {
				T[k]->init(BP, ts[k]["texture"], VK_FORMAT_R8G8B8A8_UNORM);
			} else {
				std::cout << "FORMAT UNKNOWN: " << TT << "\n";
			}
std::cout << ts[k]["id"] << "(" << k << ") " << TT << "\n";
		}

		// INSTANCES TextureCount
		nlohmann::json pis = js["instances"];
		TechniqueInstanceCount = pis.size();
std::cout << "Technique Instances count: " << TechniqueInstanceCount << "\n";
		TI = (TechniqueInstances *)calloc(TechniqueInstanceCount, sizeof(TechniqueInstances));
		InstanceCount = 0;

		for(int k = 0; k < TechniqueInstanceCount; k++) {
			std::string Pid = pis[k]["technique"].template get<std::string>();
			
			TI[k].T = TechniqueIds[Pid];
			nlohmann::json is = pis[k]["elements"];
			TI[k].InstanceCount = is.size();
std::cout << "Technique: " << Pid << "(" << k << "), Instances count: " << TI[k].InstanceCount << "\n";
			TI[k].I = (Instance *)calloc(TI[k].InstanceCount, sizeof(Instance));
			
			for(int j = 0; j < TI[k].InstanceCount; j++) {
			
std::cout << k << "." << j << "\t" << is[j]["id"] << ", " << is[j]["model"] << "(" << MeshIds[is[j]["model"]] << "), {";
				TI[k].I[j].id  = new std::string(is[j]["id"]);
				TI[k].I[j].Mid = MeshIds[is[j]["model"]];
				int NTextures = is[j]["texture"].size();
				if(NTextures != TI[k].T->Ntextures) {
					std::cout << "Wrong number of textures!\n";
					exit(0);
				}
				TI[k].I[j].NTx = NTextures;
				TI[k].I[j].Tid = (int *)calloc(NTextures, sizeof(int));
std::cout << "#" << NTextures;
				for(int h = 0; h < NTextures; h++) {
					TI[k].I[j].Tid[h] = TextureIds[is[j]["texture"][h]];
std::cout << " " << is[j]["texture"][h] << "(" << TI[k].I[j].Tid[h] << ")";
				}
std::cout << "}\n";
				nlohmann::json TMjson = is[j]["transform"];
				if(TMjson.is_null()) {
std::cout << "Node has no transform: seek for translation, rotation and scaling\n";
					bool manualPos = false;
					
					glm::vec3 trT = glm::vec3(0.0f);
					glm::mat4 trR = glm::mat4(1.0f);
					glm::vec3 trS = glm::vec3(1.0f);
					nlohmann::json Tr_Tjson = is[j]["translate"];
					if(!Tr_Tjson.is_null()) {
						trT.x = Tr_Tjson[0];
						trT.y = Tr_Tjson[1];
						trT.z = Tr_Tjson[2];
						manualPos = true;
					}
					
					nlohmann::json Tr_REjson = is[j]["eulerAngles"];
					if(!Tr_REjson.is_null()) {
						trR = glm::rotate(glm::mat4(1.0f),
										  glm::radians((float)Tr_REjson[1]),
										  glm::vec3(0.0f,1.0f,0.0f)) *
							  glm::rotate(glm::mat4(1.0f),
										  glm::radians((float)Tr_REjson[0]),
										  glm::vec3(1.0f,0.0f,0.0f)) *
							  glm::rotate(glm::mat4(1.0f),
										  glm::radians((float)Tr_REjson[2]),
										  glm::vec3(0.0f,0.0f,1.0f));
						manualPos = true;
					} else {
						nlohmann::json Tr_RQjson = is[j]["quaternion"];
						if(!Tr_RQjson.is_null()) {
							glm::quat trQ = glm::quat(Tr_RQjson[0],
													  Tr_RQjson[1],
													  Tr_RQjson[2],
													  Tr_RQjson[3]);
							trR = glm::mat4(trQ);
							manualPos = true;
						}
					}

					nlohmann::json Tr_Sjson = is[j]["scale"];
					if(!Tr_Sjson.is_null()) {
						trS.x = Tr_Sjson[0];
						trS.y = Tr_Sjson[1];
						trS.z = Tr_Sjson[2];
						manualPos = true;
					}
					
					if(manualPos) {
						TI[k].I[j].Wm = glm::translate(glm::mat4(1.0f), trT) *
										trR *
										glm::scale(glm::mat4(1.0f), trS);
					} else {
						TI[k].I[j].Wm = M[TI[k].I[j].Mid]->Wm;
std::cout << "Using model transform matrix: " << TI[k].I[j].Mid << "\n";
for(int mmm = 0; mmm < 16; mmm++) {
	std::cout << TI[k].I[j].Wm[mmm%4][mmm/4] << ", ";
}
std::cout << "\n";
					}
				} else {
					float TMj[16];
					for(int h = 0; h < 16; h++) {TMj[h] = TMjson[h];}
					TI[k].I[j].Wm = glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
				}	
				TI[k].I[j].TIp = &TI[k];
				TI[k].I[j].D = (std::vector<DescriptorSetLayout *> **)calloc(sizeof(std::vector<DescriptorSetLayout *> *), Npasses);
				TI[k].I[j].NDs = (int *)calloc(sizeof(int), Npasses);
				for(int ipas = 0; ipas < Npasses; ipas++) {
					TI[k].I[j].D[ipas] = &TI[k].T->PT[ipas].P->D;
					TI[k].I[j].NDs[ipas] = TI[k].I[j].D[ipas]->size();
					BP->DPSZs.setsInPool += TI[k].I[j].NDs[ipas];
					for(int h = 0; h < TI[k].I[j].NDs[ipas]; h++) {
						DescriptorSetLayout *DSL = (*TI[k].I[j].D[ipas])[h];
						int DSLsize = DSL->Bindings.size();

						for (int l = 0; l < DSLsize; l++) {
							if(DSL->Bindings[l].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
								BP->DPSZs.uniformBlocksInPool += 1;
							} else {
								BP->DPSZs.texturesInPool += 1;
							}
						}
					}
				}
				InstanceCount++;
			}
		}			

std::cout << "Creating instances\n";
		I =  (Instance **)calloc(InstanceCount, sizeof(Instance *));

		int i = 0;
		for(int k = 0; k < TechniqueInstanceCount; k++) {
			for(int j = 0; j < TI[k].InstanceCount; j++) {
				I[i] = &TI[k].I[j];
				InstanceIds[*I[i]->id] = i;
				I[i]->Iid = i;
				
				i++;
			}
		}
std::cout << i << " instances created\n";


/*		} catch (const nlohmann::json::exception& e) {
		std::cout << "\n\n\nException while parsing JSON file: " << file << "\n";
		std::cout << e.what() << '\n' << '\n';
		std::cout << std::flush;
		return 1;
	}
*/
//std::cout << "Leaving scene loading and creation\n";		
	return 0;
}


void Scene::pipelinesAndDescriptorSetsInit() {
//std::cout << "Scene DS init\n";
	for(int i = 0; i < InstanceCount; i++) {
//std::cout << "I: " << i << ", NTx: " << I[i]->NTx << ", NDs: " << I[i]->NDs << ", nPasses: " << Npasses << "\n";

		I[i]->DS = (DescriptorSet ***)calloc(Npasses, sizeof(DescriptorSet **));
		for(int ipas = 0; ipas < Npasses; ipas++) {
//std::cout << "DSs for pass " << ipas << ": " << I[i]->NDs[ipas] << "\n";
			I[i]->DS[ipas] = (DescriptorSet **)calloc(I[i]->NDs[ipas], sizeof(DescriptorSet *));
			for(int j = 0; j < I[i]->NDs[ipas]; j++) {
				std::vector<VkDescriptorImageInfo> Tids = {};
				TechniqueRef *Tr = I[i]->TIp->T;
				int ntxs = Tr->PT[ipas].texDefs[j].size();
				Tids.resize(ntxs);
//std::cout << "DSs " << j << " for pass " << ipas << " has " << ntxs << " textures\n";
				for(int kt = 0; kt < ntxs; kt++) {
					if(Tr->PT[ipas].texDefs[j][kt].fromInstance) {
						Tids[kt] = T[I[i]->Tid[
									  Tr->PT[ipas].texDefs[j][kt].pos
								    ]]->getViewAndSampler();
//std::cout << "Getting " << Tids[kt].sampler << " " << Tids[kt].imageView << " " << Tids[kt].imageLayout << " from insance for: i" << i << " p" << ipas << " d" << j << " t" << kt << "\n";
					} else {
						Tids[kt] = Tr->PT[ipas].texDefs[j][kt].info;
//std::cout << "Getting " << Tids[kt].sampler << " " << Tids[kt].imageView << " " << Tids[kt].imageLayout << " from technique for: i" << i << " p" << ipas << " d" << j << " t" << kt << "\n";
//						Tids[kt] = T[0]->getViewAndSampler();
					}
				}

				I[i]->DS[ipas][j] = new DescriptorSet();
//std::cout << "Allocating DS for DSL: " << (*I[i]->D[ipas])[j] << ", with " << Tids.size() << " textures\n";
				I[i]->DS[ipas][j]->init(BP, (*I[i]->D[ipas])[j], Tids);
//std::cout << "DSs " << j << " for pass " << ipas << " done!\n";
			}
		}
	}
std::cout << "Scene DS init Done\n";
}

void Scene::pipelinesAndDescriptorSetsCleanup() {
	// Cleanup datasets
	for(int i = 0; i < InstanceCount; i++) {
		for(int ipas = 0; ipas < Npasses; ipas++) {
			for(int j = 0; j < I[i]->NDs[ipas]; j++) {
				I[i]->DS[ipas][j]->cleanup();
				delete I[i]->DS[ipas][j];
			}
			free(I[i]->DS[ipas]);
		}
		free(I[i]->DS);
	}
}

void Scene::localCleanup() {
	// Cleanup textures
	for(int i = 0; i < TextureCount; i++) {
		T[i]->cleanup();
		delete T[i];
	}
	free(T);
	
	// Cleanup models
	for(int i = 0; i < ModelCount; i++) {
		M[i]->cleanup();
		delete M[i];
	}
	free(M);
	
	for(int i = 0; i < InstanceCount; i++) {
		delete I[i]->id;
		free(I[i]->Tid);
	}
	free(I);
	
	// To add: delete the also the datastructure relative to the pipeline
	for(int i = 0; i < TechniqueInstanceCount; i++) {
		free(TI[i].I);
	}
	free(TI);
}

void Scene::populateCommandBuffer(VkCommandBuffer commandBuffer, int passId, int currentImage) {
	if(passId >= Npasses) {
		std::cout << "Scene Error: requested a pass too high in scene : " << passId << " >= " << Npasses << "\n";
		exit(0);
	}
	
//std::cout << "Generating draw calls for pass " << passId << "\n";
	for(int k = 0; k < TechniqueInstanceCount; k++) {
//std::cout << "Considering technique " << k << "\n";
		for(int i = 0; i < TI[k].InstanceCount; i++) {
			Pipeline *P = TI[k].T->PT[passId].P;
			if(P != nullptr) {
				P->bind(commandBuffer);

//std::cout << "Drawing Instance " << i << "\n";
				M[TI[k].I[i].Mid]->bind(commandBuffer);
				for(int j = 0; j < TI[k].I[i].NDs[passId]; j++) {
//std::cout << "Binding DS: set " << j << "\n";
					TI[k].I[i].DS[passId][j]->bind(commandBuffer, *P, j, currentImage);
				}
//std::cout << "Draw Call\n";						
				vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(M[TI[k].I[i].Mid]->indices.size()), 1, 0, 0, 0);
			}
		}
	}
}

#endif