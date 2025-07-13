(local ffi (require :ffi))
(local {: lib} (require :assimp.lib))

(fn parse-aistr [aistr]
  (ffi.string aistr.data aistr.length))

;; TODO: GetEmbededTexture, GetEmbededTextureAndIndex
(local scene
       {:get_name (fn [self]
                    (parse-aistr self.mName))
        :has_meshes (fn [self]
                      (and self.mMeshes (> self.mNumMeshes 0)))
        :has_materials (fn [self]
                         (and self.mMaterials (> self.mNumMaterials 0)))
        :has_lights (fn [self]
                      (and self.mLights (> self.mNumLights 0)))
        :has_textures (fn [self]
                        (and self.mTextures (> self.mNumTextures 0)))
        :has_cameras (fn [self]
                       (and self.mCameras (> self.mNumCameras 0)))
        :has_animations (fn [self]
                          (and self.mAnimations (> self.mNumAnimations 0)))
        :has_skeletons (fn [self]
                         (and self.mSkeletons (> self.mNumSkeletons 0)))})

(ffi.metatype "aiScene" {:__index scene})

(fn import-file [path ?flags wrap]
  (let [scene (lib.aiImportFile path (or ?flags 0))]
    (if scene
        (wrap scene)
        (values nil (ffi.string (lib.aiGetErrorString))))))

{:import_file (λ [path ?flags]
                (import-file path ?flags
                             (fn [scene]
                               scene)))
 :import_file_nogc (λ [path ?flags]
                     (import-file path ?flags
                                  (fn [scene]
                                    (ffi.gc scene lib.aiReleaseImport))))
 :release_import (λ [scene]
                   (lib.aiReleaseImport (ffi.gc scene nil)))}
