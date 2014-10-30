using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media.Editing;
using Windows.Media.Effects;
using Windows.Storage;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using TransitionEffectTransform;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

namespace Transition_Effect
{
    public sealed partial class Playback : Page, INotifyPropertyChanged
    {
        public Playback()
        {
            this.InitializeComponent();

            LoadVideos();
            LoadEffects();

            this.DataContext = this;
        }

        private void OnPropertyChanged(string p)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(p));
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public string CurrentVideo { get; set; }
        public string SecondVideo { get; set; }
        public string ThirdVideo { get; set; }
        public TransitionEffect CurrentEffect { get; set; }

        public ObservableCollection<string> VideoList { get; set; }
        public ObservableCollection<TransitionEffect> EffectList { get; set; }

        private void LoadEffects()
        {
            EffectList = new ObservableCollection<TransitionEffect>();
            //EffectList.Add(new TransitionEffect() { Name = "Black", EffectType = TransitionEffectType.TRANSITION_BLACK });
            //EffectList.Add(new TransitionEffect() { Name = "White", EffectType = TransitionEffectType.TRANSITION_WHITE });
            EffectList.Add(new TransitionEffect() { Name = "Black Fading", EffectType = TransitionEffectType.TRANSITION_FADE_TO_BLACK });
            EffectList.Add(new TransitionEffect() { Name = "White Fading", EffectType = TransitionEffectType.TRANSITION_FADE_TO_WHITE });
            //EffectList.Add(new TransitionEffect() { Name = "Fading Transit", EffectType = TransitionEffectType.TRANSITION_FADE_OVERLAP });
            EffectList.Add(new TransitionEffect() { Name = "Right Left", EffectType = TransitionEffectType.TRANSITION_RIGHT_TO_LEFT });
            EffectList.Add(new TransitionEffect() { Name = "Top Bottom", EffectType = TransitionEffectType.TRANSITION_TOP_TO_BOTTOM });
            EffectList.Add(new TransitionEffect() { Name = "Bottom Top", EffectType = TransitionEffectType.TRANSITION_BOTTOM_TO_TOP });
            EffectList.Add(new TransitionEffect() { Name = "ZoomIn", EffectType = TransitionEffectType.TRANSITION_ROOM_IN });
            //EffectList.Add(new TransitionEffect() { Name = "ZoomOut", EffectType = TransitionEffectType.TRANSITION_ROOM_OUT });
        }

        private void LoadVideos()
        {
            VideoList = new ObservableCollection<string>();

            VideoList.Add(@"ms-appx:///Assets/Videos/1.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/big_buck_bunny_trailer_480p_high.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4");

        }

        private async void Apply_Effect(object sender, RoutedEventArgs e)
        {
            if (CurrentVideo == null || CurrentEffect == null)
                return;

            SecondVideo = VideoList.IndexOf(CurrentVideo) == VideoList.Count - 1 ? VideoList[0] : VideoList[VideoList.IndexOf(CurrentVideo) + 1];
            ThirdVideo = VideoList.IndexOf(SecondVideo) == VideoList.Count - 1 ? VideoList[0] : VideoList[VideoList.IndexOf(SecondVideo) + 1];

            MediaComposition comp = new MediaComposition();

            StorageFile VideoFile1 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(CurrentVideo));
            StorageFile VideoFile2 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(SecondVideo));
            StorageFile VideoFile3 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(ThirdVideo));
            MediaClip mediaClip1 = await MediaClip.CreateFromFileAsync(VideoFile1);
            MediaClip mediaClip2 = await MediaClip.CreateFromFileAsync(VideoFile2);
            MediaClip mediaClip3 = await MediaClip.CreateFromFileAsync(VideoFile3);

            TransitionEffectParameter transitionEffectParameter1 = new TransitionEffectParameter();
            TransitionEffectParameter transitionEffectParameter2 = new TransitionEffectParameter();
            TransitionEffectParameter transitionEffectParameter3 = new TransitionEffectParameter();

            transitionEffectParameter1.SetStartEffect(TransitionEffectType.TRANSITION_NONE, 0);
            transitionEffectParameter1.SetEndEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter1.SetVideoDuration((float)mediaClip1.OriginalDuration.TotalSeconds);

            transitionEffectParameter2.SetStartEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter2.SetEndEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter2.SetVideoDuration((float)mediaClip2.OriginalDuration.TotalSeconds);

            transitionEffectParameter3.SetStartEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter3.SetEndEffect(TransitionEffectType.TRANSITION_NONE, 0);
            transitionEffectParameter3.SetVideoDuration((float)mediaClip3.OriginalDuration.TotalSeconds);

            PropertySet configuration1 = new PropertySet();
            configuration1.Add("TransitionEffectParameter", transitionEffectParameter1);
            mediaClip1.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration1));

            PropertySet configuration2 = new PropertySet();
            configuration2.Add("TransitionEffectParameter", transitionEffectParameter2);
            mediaClip2.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration2));

            PropertySet configuration3 = new PropertySet();
            configuration3.Add("TransitionEffectParameter", transitionEffectParameter3);
            mediaClip3.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration3));

            comp.Clips.Add(mediaClip1);
            comp.Clips.Add(mediaClip2);
            comp.Clips.Add(mediaClip3);

            Video.SetMediaStreamSource(comp.GeneratePreviewMediaStreamSource(320, 240));
        }

        private async void Save_Click(object sender, RoutedEventArgs e)
        {
            Video.Stop();
            Video.Source = null;
            Video.RemoveAllEffects();

            if (CurrentVideo == null || CurrentEffect == null || SecondVideo == null || ThirdVideo == null)
                return;

            MediaComposition comp = new MediaComposition();

            StorageFile VideoFile1 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(CurrentVideo));
            StorageFile VideoFile2 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(SecondVideo));
            StorageFile VideoFile3 = await StorageFile.GetFileFromApplicationUriAsync(new Uri(ThirdVideo));
            MediaClip mediaClip1 = await MediaClip.CreateFromFileAsync(VideoFile1);
            MediaClip mediaClip2 = await MediaClip.CreateFromFileAsync(VideoFile2);
            MediaClip mediaClip3 = await MediaClip.CreateFromFileAsync(VideoFile3);

            TransitionEffectParameter transitionEffectParameter1 = new TransitionEffectParameter();
            TransitionEffectParameter transitionEffectParameter2 = new TransitionEffectParameter();
            TransitionEffectParameter transitionEffectParameter3 = new TransitionEffectParameter();

            transitionEffectParameter1.SetStartEffect(TransitionEffectType.TRANSITION_NONE, 0);
            transitionEffectParameter1.SetEndEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter1.SetVideoDuration((float)mediaClip1.OriginalDuration.TotalSeconds);

            transitionEffectParameter2.SetStartEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter2.SetEndEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter2.SetVideoDuration((float)mediaClip2.OriginalDuration.TotalSeconds);

            transitionEffectParameter3.SetStartEffect(CurrentEffect.EffectType, 1);
            transitionEffectParameter3.SetEndEffect(TransitionEffectType.TRANSITION_NONE, 0);
            transitionEffectParameter3.SetVideoDuration((float)mediaClip3.OriginalDuration.TotalSeconds);

            PropertySet configuration1 = new PropertySet();
            configuration1.Add("TransitionEffectParameter", transitionEffectParameter1);
            mediaClip1.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration1));  //change it to EffectTransform 

            PropertySet configuration2 = new PropertySet();
            configuration2.Add("TransitionEffectParameter", transitionEffectParameter2);
            mediaClip2.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration2));  //change it to EffectTransform 

            PropertySet configuration3 = new PropertySet();
            configuration3.Add("TransitionEffectParameter", transitionEffectParameter3);
            mediaClip3.VideoEffectDefinitions.Add(new VideoEffectDefinition("TransitionEffectTransform.TransitionEffect", configuration3));  //change it to EffectTransform 

            comp.Clips.Add(mediaClip1);
            comp.Clips.Add(mediaClip2);
            comp.Clips.Add(mediaClip3);

            StorageFile destination = await ApplicationData.Current.LocalFolder.CreateFileAsync("transition_result.mp4", CreationCollisionOption.ReplaceExisting);

            Save.IsEnabled = false;

            var a = comp.RenderToFileAsync(destination);
            a.Progress = new AsyncOperationProgressHandler<Windows.Media.Transcoding.TranscodeFailureReason, double>(tempProgress);
            a.Completed = new AsyncOperationWithProgressCompletedHandler<Windows.Media.Transcoding.TranscodeFailureReason, double>(tempCOmpleted);
        }

        private string progress = "start";

        public string Prog
        {
            get { return progress; }
            set
            {
                progress = value;
                OnPropertyChanged("Prog");
            }
        }

        private async void tempCOmpleted(IAsyncOperationWithProgress<Windows.Media.Transcoding.TranscodeFailureReason, double> asyncInfo, AsyncStatus asyncStatus)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.High, () =>
            {
                MessageDialog m = new MessageDialog("Save Completed");
                m.ShowAsync();

                Save.IsEnabled = true;

                if (asyncInfo.ErrorCode != null)
                    Prog = asyncInfo.ErrorCode.Message;
                else
                {
                    Prog = "Completed";
                }
            });
        }

        private async void tempProgress(IAsyncOperationWithProgress<Windows.Media.Transcoding.TranscodeFailureReason, double> asyncInfo, double progressInfo)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.High, () =>
            {
                Prog = progressInfo.ToString();
            });

        }

        private void Back_Click(object sender, RoutedEventArgs e)
        {
            App.RootFrame.Navigate(typeof(MainPage));
        }
    }
}
